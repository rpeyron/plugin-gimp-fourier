/**
 *  (c) 2002-2022 - Remi Peyronnet  (see README.md for contributors and changelog)
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
 *  You'll need to install fftw version 3
 *
 *  Minimal install command:
 *
 *  CFLAGS="-O2" LIBS="-L/usr/local/lib -lfftw3" gimptool --install fourier.c
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
#define VERSION "0.4.5"
#endif

/** Defines ******************************************************************/

#define PLUG_IN_NAME "plug_in_fft"
#define PLUG_IN_VERSION "Mar 2024, " VERSION

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


#include "fourier_common.inc"

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
  GeglRectangle *roi;
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

  // Ensure selection does not exceed image
  if (sel_x1 < 0) sel_x1 = 0; if (sel_x1 > img_width) sel_x1 = img_width;
  if (sel_y1 < 0) sel_y1 = 0; if (sel_y1 > img_height) sel_y1 = img_height;
  if (sel_x2 < 0) sel_x2 = 0; if (sel_x2 > img_width) sel_x2 = img_width;
  if (sel_y2 < 0) sel_y2 = 0; if (sel_y2 > img_height) sel_y2 = img_height;

  sel_width = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  //printf("Image size %dx%d - %d bpp\n", img_width, img_height, img_bpp);
  //printf("Selection size %dx%d (%d,%d-%d,%d)\n", sel_width, sel_height, sel_x1, sel_y1, sel_x2, sel_y2);

  if (status == GIMP_PDB_SUCCESS)
  {
    gimp_progress_init(fft_inv ? "Applying inverse Fourier transform..." : "Applying forward Fourier transform...");

    // Init buffers
    GeglBuffer *src_buffer = gimp_drawable_get_buffer(drawable_id);
    GeglBuffer *dest_buffer = gimp_drawable_get_shadow_buffer(drawable_id);
    
    roi = GEGL_RECTANGLE(sel_x1, sel_y1, sel_width, sel_height);
    img_pixels = g_malloc(roi->width * roi->height * img_bpp);

    // Get source image
    gegl_buffer_get(src_buffer, roi, 1.0, format, img_pixels, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    if (fft_inv == 0)
    {
      process_fft_forward(img_pixels, img_pixels, sel_width, sel_height, img_bpp, img_bpp);
    }
    else
    {
      process_fft_inverse(img_pixels, img_pixels, sel_width, sel_height, img_bpp, img_bpp);
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
