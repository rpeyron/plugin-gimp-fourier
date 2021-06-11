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

/** Defines ******************************************************************/

#define PLUG_IN_NAME "plug_in_fft"
#define PLUG_IN_VERSION "Jan. 2010, 0.4.1"

/** Plugin interface *********************************************************/

static void query(void);
static void run (const gchar      *name,
                 gint              nparams,
                 const GimpParam  *param,
                 gint             *nreturn_vals,
                 GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

/** Conversion functions *****************************************************/

inline gint round_gint (double value)
{
  double floored = floor (value);
  if (value - floored > 0.5)
  {
    return (gint)(floored + 1);
  }
  return (gint)floored;
}

inline gint boost (double value)
{
  double bounded = fabs (value / 160.0);
  gint boosted = round_gint (128.0 * sqrt (bounded));
  boosted = (value > 0) ? boosted : -boosted;
  return boosted;
}

inline double unboost (double value)
{
  double bounded = fabs (value / 128.0);
  double unboosted = 160.0 * bounded * bounded;
  unboosted = (value > 0) ? unboosted : -unboosted;
  return unboosted;
}

inline guchar get_guchar (gint x, gint y, double d)
{
  gint i = round_gint (d);
  //if (i > 255 || i < 0) { printf(" (%d, %d: %d) ", x, y, i); }
  return (guchar) (i>=255)?255:((i<0)?0:i);
}

inline guchar get_gchar128 (gint x, gint y, gint i)
{
  //if (i > 127 || i < -128) { printf(" (%d, %d: %d) ", x, y, i); }
  return (guchar) (i>=(gint)128)?255:((i<=(gint)-128)?0:i+128);
}

inline double get_double128 (gint x, gint y, guchar c)
{
  return (double)(c) - 128.0;
}

/* Should pixel store imaginary part? */
static inline gint pixel_imag(gint row, gint col, gint h, gint w)
{
  if (row==0 && h%2==0 || row==h/2) return col>w/2;
  else return row>h/2;
}

/*
 * Map images coordinates (row, col) into Fourier array indices (row2, col2).
 */
static inline void map(gint row, gint col, gint h, gint w,
    gint *row2, gint *col2)
{
  *row2 = (row+(h+1)/2) % h;              /* shift origin */
  *col2 = (col+(w+1)/2) % w;
  if (*col2 > w/2) {                      /* wrap */
    *row2 = (h-*row2)%h;
    *col2 = w-*col2;
  }
  *col2 *= 2;                             /* unit = real number */
  if (pixel_imag(row, col, h, w)) (*col2)++;  /* take imaginary part */
}

inline double normalize (gint x, gint y, gint width, gint height)
{
  double cx = (double)abs(x - width/2);
  double cy = (double)abs(y - height/2);
  double energy = (sqrt(cx)+sqrt(cy));
  return energy*energy;
}

/** Main GIMP functions ******************************************************/

MAIN()

void query(void)
{
  /* Definition of parameters */
  static GimpParamDef args[] = {
    { GIMP_PDB_INT32, (gchar *)"run_mode", (gchar *)"Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, (gchar *)"image", (gchar *)"Input image (unused)" },
    { GIMP_PDB_DRAWABLE, (gchar *)"drawable", (gchar *)"Input drawable" }
  };

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
    G_N_ELEMENTS (args), 0,
    args, NULL);
  gimp_plugin_menu_register ("plug_in_fft_dir", "<Image>/Filters/Generic");
  
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
    G_N_ELEMENTS (args), 0,
    args, NULL);
  gimp_plugin_menu_register ("plug_in_fft_inv", "<Image>/Filters/Generic");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  /* Return values */
  static GimpParam values[1];

  gint sel_x1, sel_y1, sel_x2, sel_y2, w, h, padding;
  gint img_height, img_width, img_bpp, cur_bpp, img_has_alpha;

  GimpDrawable      *drawable;
  GimpPixelRgn       region;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;

  gint progress, max_progress;

  gint row, col, row2, col2, bounded;

  fftw_plan p;
  double v, norm;
  double *fft_real;

  int fft_inv=0;

  if (strcmp(name,"plug_in_fft_inv") == 0) { fft_inv = 1; }

  *nreturn_vals = 1;
  *return_vals  = values;

  status = GIMP_PDB_SUCCESS;

  if (param[0].type!= GIMP_PDB_INT32)  status=GIMP_PDB_CALLING_ERROR;
  if (param[2].type!=GIMP_PDB_DRAWABLE)   status=GIMP_PDB_CALLING_ERROR;

  run_mode = (GimpRunMode) param[0].data.d_int32;

  drawable = gimp_drawable_get(param[2].data.d_drawable);

  img_width     = gimp_drawable_width(drawable->drawable_id);
  img_height    = gimp_drawable_height(drawable->drawable_id);
  img_bpp       = gimp_drawable_bpp(drawable->drawable_id);
  img_has_alpha = gimp_drawable_has_alpha(drawable->drawable_id);
  gimp_drawable_mask_bounds(drawable->drawable_id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
  // printf("img_width: %d\n", img_width);
  // printf("img_height: %d\n", img_height);
  // printf("img_bpp: %d\n", img_bpp);
  // printf("img_has_alpha: %d\n", img_has_alpha);

  w = sel_x2 - sel_x1;
  h = sel_y2 - sel_y1;

  if (status == GIMP_PDB_SUCCESS)
  {
    guchar * img_pixels;

    gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

    gimp_progress_init(fft_inv?"Apply inverse Fourier transform...":"Apply Fourier transform...");

    // Process
    gimp_pixel_rgn_init (&region, drawable, sel_x1, sel_y1, w, h, FALSE, FALSE);
    img_pixels = g_new (guchar, w * h * img_bpp );
    gimp_pixel_rgn_get_rect(&region, img_pixels, sel_x1, sel_y1, w, h);

    gimp_pixel_rgn_init (&region, drawable, sel_x1, sel_y1, w, h, TRUE, TRUE);

    // FFT !
    padding = (w&1) ? 1 : 2;
    
    fft_real = g_new(double, (w+padding) * h);
    
    max_progress = /*w*h*/img_bpp*3;

    // printf("Making plans...\n");
    if (fft_inv == 0)
    {
      p = fftw_plan_dft_r2c_2d(h, w, fft_real, (fftw_complex *) fft_real, FFTW_ESTIMATE);
    }
    else
    {
      p = fftw_plan_dft_c2r_2d(h, w, (fftw_complex *) fft_real, fft_real, FFTW_ESTIMATE);
    }
    // printf("Done!\n");
    
    progress = 0;

    for(cur_bpp=0;cur_bpp<img_bpp;cur_bpp++)
    {
    if (fft_inv == 0)
    {
      for(col=0;col<w;col++)
      {
        for(row=0;row<h;row++)
        {
           v = (double)img_pixels[(row*w+col)*img_bpp+cur_bpp];
           fft_real[row*(w+padding)+col] = v;
        }
      }
      progress += 1;
      gimp_progress_update((double) progress / max_progress);
      fftw_execute(p);
      progress += 1;
      gimp_progress_update((double) progress / max_progress);
    
      for(row = 0; row < h; row++)
      {
        for(col = 0; col < w; col++)
        {
          map(row, col, h, w, &row2, &col2);
          v = fft_real [row2*(w+padding) + col2] / (double)(w * h);
          norm = normalize (col, row, w, h);
          bounded = boost (v * norm);
          img_pixels[(row*w+col)*img_bpp+cur_bpp] = get_gchar128 (col, row, bounded);
        }
      }
      // do not boost (0, 0), just offset it
      row = h/2;
      col = w/2;
      bounded = round_gint ((fft_real [0] / (double)(w * h)) - 128.0);
      img_pixels [(row*w+col)*img_bpp + cur_bpp] = get_gchar128 (col, row, bounded);
      progress += 1;
      gimp_progress_update((double) progress / max_progress);
    }
    else
    {
      for (row = 0; row < h; row++)
      {
        for(col = 0; col < w; col++)
        {
          map(row, col, h, w, &row2, &col2);
          norm = normalize (col, row, w, h);
          v = get_double128(row, col, img_pixels[(row*w+col)*img_bpp+cur_bpp]);
          fft_real[row2*(w+padding)+col2] = unboost (v) / norm;
        }
      }
      // restore redundancy
      for (col2 = 0; col2 < w+padding; col2 += (w+1)/2*2) {
        for (row2 = 1; row2 < (h+1)/2; row2++) {
          fft_real[(h-row2)*(w+padding)+col2+1] = -fft_real[row2*(w+padding)+col2+1];
          fft_real[row2*(w+padding)+col2] = fft_real[(h-row2)*(w+padding)+col2];
        }
        fft_real[col2+1] = 0;
        if (h%2 == 0) fft_real[h/2*(w+padding)+col2+1] = 0;
      }
      // do not unboost (0, 0), just offset it
      row = h/2;
      col = w/2;
      v = get_double128 (row, col, img_pixels [(row*w + col)*img_bpp + cur_bpp]);
      fft_real [0] = v + 128.0;

      progress += 1;
      gimp_progress_update((double) progress / max_progress);
      fftw_execute(p);
      progress += 1;
      gimp_progress_update((double) progress / max_progress);
      for(col = 0; col < w; col++)
      {
        for(row = 0; row < h; row++)
        {
          v = fft_real[row*(w+padding)+col];
          img_pixels[(row*w+col)*img_bpp+cur_bpp] = get_guchar(col, row, v);
        }
      }
      progress += 1;
      gimp_progress_update((double) progress / max_progress);
    }
    }
    fftw_destroy_plan(p);
    g_free(fft_real);

    // Flush

    gimp_pixel_rgn_set_rect(&region, img_pixels, sel_x1, sel_y1,
                            (sel_x2-sel_x1), (sel_y2-sel_y1));
    g_free (img_pixels);

    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
    gimp_drawable_update (drawable->drawable_id, sel_x1, sel_y1, (sel_x2-sel_x1), (sel_y2-sel_y1));
    gimp_displays_flush();

    // set FG to neutral grey; used to mask moire patterns, etc
    if (fft_inv == 0)
    {
      GimpRGB neutral_grey;
      gimp_rgba_set_uchar(&neutral_grey, 128, 128, 128, 1);
      gimp_context_set_foreground(&neutral_grey);
    }

  }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  gimp_drawable_detach(drawable);

}
