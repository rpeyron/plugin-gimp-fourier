/**
 *  (c) 2002-2009 - Remi Peyronnet
 *
 *  Plugin GIMP : Fourier Transform
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *  Aug 2022: Remplaced deprecated functions
 *  Jan 2010: Select Gray after transform + doc (patch by Martin Ramshaw)
 *  Oct 2009: Reordered the data in a more natural way:
 *            no Fourier coefficient is lost (patch by Edgar Bonet)
 *  Feb 2009: Fixed Makefile by using pkg-config instead of gimptool
 *  Jan 2009: Officialized distribution under GPL
 *  Dec 2007: Zero initialize padding (patch by Rene Rebe)
 *  Mar 2005: Windows compatibility, inverse remove parasite, cosmetics
 *  Aug 2005: Normalization by alejandrofer at google mail.com
 *            Remove parasite, normalize by a power function
 *  Mar 2005: Moved to gimp-2.2 by mk@crc.dk:
 *            Handles RGB and grayscale images
 *            Scale factors stored as parasite information
 *            Columns are swapped
 *  Oct 2004: Moved to gimp-2.0 (Linux only)
 *  May 2002: Minor modifications by mk@crc.dk
 *
 *  You'll need to install fftw version 3
 *
 *  To install, run:
 *
 *  CFLAGS="-O2" LIBS="-L/usr/local/lib -lfftw3" gimptool --install gpplugin.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <libgimp/gimp.h>

// Uses the brillant fftw lib
#include <fftw3.h>

#if __has_include("fourier-config.h")
#include "fourier-config.h"
#else
#define VERSION "0.4.4"
#endif

/** Defines ******************************************************************/

#define PLUG_IN_NAME "plug_in_fft"
#define PLUG_IN_VERSION "Aug 2022, " VERSION

/** Plugin interface *********************************************************/

static void query(void);
static void run(const gchar *name,
                gint nparams,
                const GimpParam *param,
                gint *nreturn_vals,
                GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run    /* run_proc   */
};

/** Conversion functions *****************************************************/

inline gint round_gint(double value)
{
  double floored = floor(value);
  if (value - floored > 0.5)
  {
    return (gint)(floored + 1);
  }
  return (gint)floored;
}

inline gint boost(double value)
{
  double bounded = fabs(value / 160.0);
  gint boosted = round_gint(128.0 * sqrt(bounded));
  boosted = (value > 0) ? boosted : -boosted;
  return boosted;
}

inline double unboost(double value)
{
  double bounded = fabs(value / 128.0);
  double unboosted = 160.0 * bounded * bounded;
  unboosted = (value > 0) ? unboosted : -unboosted;
  return unboosted;
}

inline guchar get_guchar(gint x, gint y, double d)
{
  gint i = round_gint(d);
  // if (i > 255 || i < 0) { printf(" (%d, %d: %d) ", x, y, i); }
  return (guchar)(i >= 255) ? 255 : ((i < 0) ? 0 : i);
}

inline guchar get_gchar128(gint x, gint y, gint i)
{
  // if (i > 127 || i < -128) { printf(" (%d, %d: %d) ", x, y, i); }
  return (guchar)(i >= (gint)128) ? 255 : ((i <= (gint)-128) ? 0 : i + 128);
}

inline double get_double128(gint x, gint y, guchar c)
{
  return (double)(c)-128.0;
}

/* Should pixel store imaginary part? */
static inline gint pixel_imag(gint row, gint col, gint h, gint w)
{
  if (row == 0 && h % 2 == 0 || row == h / 2)
    return col > w / 2;
  else
    return row > h / 2;
}

/*
 * Map images coordinates (row, col) into Fourier array indices (row2, col2).
 */
static inline void map(gint row, gint col, gint h, gint w,
                       gint *row2, gint *col2)
{
  *row2 = (row + (h + 1) / 2) % h; /* shift origin */
  *col2 = (col + (w + 1) / 2) % w;
  if (*col2 > w / 2)
  { /* wrap */
    *row2 = (h - *row2) % h;
    *col2 = w - *col2;
  }
  *col2 *= 2; /* unit = real number */
  if (pixel_imag(row, col, h, w))
    (*col2)++; /* take imaginary part */
}

inline double normalize(gint x, gint y, gint width, gint height)
{
  double cx = (double)abs(x - width / 2);
  double cy = (double)abs(y - height / 2);
  double energy = (sqrt(cx) + sqrt(cy));
  return energy * energy;
}

/** Process Functions *********************************************************/

void process_fft_forward(guchar *img_pixels, gint sel_width, gint sel_height, gint img_bpp)
{

  gint row, col, row2, col2, cur_bpp, bounded, padding;
  gint progress, max_progress;
  fftw_plan p;
  double v, norm;
  double *fft_real;

  padding = (sel_width & 1) ? 1 : 2;

  fft_real = g_new(double, (sel_width + padding) * sel_height);

  progress = 0;
  max_progress = img_bpp * 3;

  p = fftw_plan_dft_r2c_2d(sel_height, sel_width, fft_real, (fftw_complex *)fft_real, FFTW_ESTIMATE);

  for (cur_bpp = 0; cur_bpp < img_bpp; cur_bpp++)
  {
    for (col = 0; col < sel_width; col++)
    {
      for (row = 0; row < sel_height; row++)
      {
        v = (double)img_pixels[(row * sel_width + col) * img_bpp + cur_bpp];
        fft_real[row * (sel_width + padding) + col] = v;
      }
    }
    gimp_progress_update((double)++progress / max_progress);
    fftw_execute(p);
    gimp_progress_update((double)++progress / max_progress);

    for (row = 0; row < sel_height; row++)
    {
      for (col = 0; col < sel_width; col++)
      {
        map(row, col, sel_height, sel_width, &row2, &col2);
        v = fft_real[row2 * (sel_width + padding) + col2] / (double)(sel_width * sel_height);
        norm = normalize(col, row, sel_width, sel_height);
        bounded = boost(v * norm);
        img_pixels[(row * sel_width + col) * img_bpp + cur_bpp] = get_gchar128(col, row, bounded);
      }
    }
    // do not boost (0, 0), just offset it
    row = sel_height / 2;
    col = sel_width / 2;
    bounded = round_gint((fft_real[0] / (double)(sel_width * sel_height)) - 128.0);
    img_pixels[(row * sel_width + col) * img_bpp + cur_bpp] = get_gchar128(col, row, bounded);
    gimp_progress_update((double)++progress / max_progress);
  }

  fftw_destroy_plan(p);
  g_free(fft_real);
}

void process_fft_inverse(guchar *img_pixels, gint sel_width, gint sel_height, gint img_bpp)
{

  gint row, col, row2, col2, cur_bpp, bounded, padding;
  gint progress, max_progress;
  fftw_plan p;
  double v, norm;
  double *fft_real;

  padding = (sel_width & 1) ? 1 : 2;

  fft_real = g_new(double, (sel_width + padding) * sel_height);

  progress = 0;
  max_progress = img_bpp * 3;

  p = fftw_plan_dft_c2r_2d(sel_height, sel_width, (fftw_complex *)fft_real, fft_real, FFTW_ESTIMATE);

  for (cur_bpp = 0; cur_bpp < img_bpp; cur_bpp++)
  {
    for (row = 0; row < sel_height; row++)
    {
      for (col = 0; col < sel_width; col++)
      {
        map(row, col, sel_height, sel_width, &row2, &col2);
        norm = normalize(col, row, sel_width, sel_height);
        v = get_double128(row, col, img_pixels[(row * sel_width + col) * img_bpp + cur_bpp]);
        fft_real[row2 * (sel_width + padding) + col2] = unboost(v) / norm;
      }
    }
    // restore redundancy
    for (col2 = 0; col2 < sel_width + padding; col2 += (sel_width + 1) / 2 * 2)
    {
      for (row2 = 1; row2 < (sel_height + 1) / 2; row2++)
      {
        fft_real[(sel_height - row2) * (sel_width + padding) + col2 + 1] = -fft_real[row2 * (sel_width + padding) + col2 + 1];
        fft_real[row2 * (sel_width + padding) + col2] = fft_real[(sel_height - row2) * (sel_width + padding) + col2];
      }
      fft_real[col2 + 1] = 0;
      if (sel_height % 2 == 0)
        fft_real[sel_height / 2 * (sel_width + padding) + col2 + 1] = 0;
    }
    // do not unboost (0, 0), just offset it
    row = sel_height / 2;
    col = sel_width / 2;
    v = get_double128(row, col, img_pixels[(row * sel_width + col) * img_bpp + cur_bpp]);
    fft_real[0] = v + 128.0;

    gimp_progress_update((double)++progress / max_progress);
    fftw_execute(p);
    gimp_progress_update((double)++progress / max_progress);
    for (col = 0; col < sel_width; col++)
    {
      for (row = 0; row < sel_height; row++)
      {
        v = fft_real[row * (sel_width + padding) + col];
        img_pixels[(row * sel_width + col) * img_bpp + cur_bpp] = get_guchar(col, row, v);
      }
    }
    gimp_progress_update((double)++progress / max_progress);
  }

  fftw_destroy_plan(p);
  g_free(fft_real);
}

/** Main GIMP functions ******************************************************/


MAIN()

void query(void)
{
  /* Definition of parameters */
  static GimpParamDef args[] = {
      {GIMP_PDB_INT32, (gchar *)"run_mode", (gchar *)"Interactive, non-interactive"},
      {GIMP_PDB_IMAGE, (gchar *)"image", (gchar *)"Input image (unused)"},
      {GIMP_PDB_DRAWABLE, (gchar *)"drawable", (gchar *)"Input drawable"}};

  /* Forward FFT */
  gimp_install_procedure(
      "plug_in_fft_dir",
      "Apply an FFT to the image. This can remove (for example) moire patterns from images scanned from books:\n\n    The image should be RGB (Image|Mode|RGB)\n\n    Remove the alpha layer, if present (Image|Flatten Image)\n\n    Select Filters|Generic|FFT Forward\n\n    Use the preselected neutral grey to effectively remove any moir patterns from the image. Either paint over any patterns or\n\n     - In the Layers window, select the layer, and 'Duplicate Layer'\n     - Select Colours|Brightness-Contrast. Increase the Contrast to see any patterns.\n     - Use the Rectangular and/or Elliptical Selection tools to select any patterns on the contrast layer.\n     - Then remove the contrast layer leaving the original FFT layer with the selections.\n     - Then select Edit|Fill with FG colour, remembering to cancel the Selection afterwards!\n\n    Select Filters|Generic|FFT Inverse\n\nVoila, an image without the moire pattern!",
      "This plug-in applies a FFT to the image, for educationnal or effects purpose.",
      "Remi Peyronnet",
      "Remi Peyronnet",
      PLUG_IN_VERSION,
      "FFT Forward",
      "RGB*, GRAY*",
      GIMP_PLUGIN,
      G_N_ELEMENTS(args), 0,
      args, NULL);
  gimp_plugin_menu_register("plug_in_fft_dir", "<Image>/Filters/Generic");

  /* Inverse FFT */
  gimp_install_procedure(
      "plug_in_fft_inv",
      "Apply an inverse FFT to the image, effectively restoring the original image (plus changes).",
      "This plug-in applies a FFT to the image, for educationnal or effects purpose.",
      "Remi Peyronnet",
      "Remi Peyronnet",
      PLUG_IN_VERSION,
      "FFT Inverse",
      "RGB*, GRAY*",
      GIMP_PLUGIN,
      G_N_ELEMENTS(args), 0,
      args, NULL);
  gimp_plugin_menu_register("plug_in_fft_inv", "<Image>/Filters/Generic");
}

static void
run(const gchar *name,
    gint nparams,
    const GimpParam *param,
    gint *nreturn_vals,
    GimpParam **return_vals)
{
  /* Return values */
  static GimpParam values[1];

  gint sel_x1, sel_y1, sel_x2, sel_y2, sel_width, sel_height, padding;
  gint img_height, img_width, img_bpp, img_has_alpha;

  gint32 drawable_id;
  GimpDrawable *drawable;
  GimpRunMode run_mode;
  GimpPDBStatusType status;
  const Babl *format;

  GeglBuffer *buffer;
  guchar *img_pixels;

  int fft_inv = 0;

  if (strcmp(name, "plug_in_fft_inv") == 0)
  {
    fft_inv = 1;
  }

  *nreturn_vals = 1;
  *return_vals = values;

  status = GIMP_PDB_SUCCESS;

  if (param[0].type != GIMP_PDB_INT32)
    status = GIMP_PDB_CALLING_ERROR;
  if (param[2].type != GIMP_PDB_DRAWABLE)
    status = GIMP_PDB_CALLING_ERROR;

  run_mode = (GimpRunMode)param[0].data.d_int32;

  
  gegl_init (NULL, NULL);

  drawable_id = param[2].data.d_drawable;

  img_width = gimp_drawable_width(drawable_id);
  img_height = gimp_drawable_height(drawable_id);
  // img_bpp = gimp_drawable_get_bpp(drawable_id);
  img_has_alpha = gimp_drawable_has_alpha(drawable_id);

  if (gimp_drawable_has_alpha(drawable_id)) //  gimp_drawable_is_rgb (drawable)
    format = babl_format("R'G'B'A u8");
  else
    format = babl_format("R'G'B' u8");

  img_bpp = babl_format_get_bytes_per_pixel(format);

  gimp_drawable_mask_bounds(drawable_id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  if (status == GIMP_PDB_SUCCESS)
  {
    gimp_progress_init(fft_inv ? "Applying inverse Fourier transform..." : "Applying forward Fourier transform...");

    // Init buffers
    GeglBuffer *src_buffer = gimp_drawable_get_buffer(drawable_id);
    GeglBuffer *dest_buffer = gimp_drawable_get_shadow_buffer(drawable_id);
    img_pixels = g_new(guchar, sel_width * sel_height * img_bpp);

    // Get source image
    gegl_buffer_get(src_buffer, GEGL_RECTANGLE(sel_x1, sel_y1, sel_x2, sel_y2), 1.0,
                    format, img_pixels,
                    GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    if (fft_inv == 0)
    {
      process_fft_forward(img_pixels, sel_width, sel_height, img_bpp);
    }
    else
    {
      process_fft_inverse(img_pixels, sel_width, sel_height, img_bpp);
    }

    // Set result to image
    gegl_buffer_set(dest_buffer, GEGL_RECTANGLE(sel_x1, sel_y1, sel_x2, sel_y2), 0,
                    format, img_pixels,
                    GEGL_AUTO_ROWSTRIDE);

    g_free(img_pixels);
    g_object_unref(src_buffer);
    g_object_unref(dest_buffer);

    gimp_drawable_merge_shadow(drawable_id, TRUE);
    gimp_drawable_update(drawable_id, sel_x1, sel_y1, (sel_x2 - sel_x1), (sel_y2 - sel_y1));
    gimp_displays_flush();

    // set FG to neutral grey; used to mask moire patterns, etc
    if (fft_inv == 0)
    {
      GimpRGB neutral_grey;
      gimp_rgba_set_uchar(&neutral_grey, 128, 128, 128, 1);
      gimp_context_set_foreground(&neutral_grey);
    }

    gimp_progress_init(fft_inv ? "Inverse Fourier transform applied successfully." : "Forward Fourier transform applied successfully.");

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;
  }
}
