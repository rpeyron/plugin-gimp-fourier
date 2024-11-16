/**
 *  (c) 2002-2024 - Remi Peyronnet  (see README.md for contributors and changelog)
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

// msys2 -mingw64 -c 'echo $(gimptool-2.99 -n --build fourier.c) -lfftw3 -O3 | sh'

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

// GIMP headers
#include <libgimp/gimp.h>

// Uses the brillant fftw lib
#include <fftw3.h>

// Plugin Config
#if __has_include("fourier-config.h")
#include "fourier-config.h"
#else
#define VERSION "0.4.5"
#define GETTEXT_PACKAGE3 "gimp30-fourier"
#endif

/**
 * Note about translation strings:
 *
 * * For small strings or strings used only once:
 *   - #define MYSTRING "my string"
 *   - use at runtime with _(MYSTRING)
 *
 * * For larger strings or used several time:
 *   - static const char *MYSTRING = d_("my string");  // Needed for xgettex to extract the string
 *   - use at runtime with _(MYSTRING)   // To actually translate the string after gettext has been initialized
 *  please note that both should be synchronized to extract all strings that needs to be translated,
 *  but to avoid to extract strings that do not need to be translated
 *
 */

// To extract strings defined as static const char *  (and used later with _)
#define d_(String) String

#if (GIMP_MAJOR_VERSION == 3) || ((GIMP_MAJOR_VERSION == 2) && (GIMP_MINOR_VERSION >= 99))
#ifdef HAVE_GETTEXT
#include <libintl.h>
#include <locale.h>
#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif
#define _(String) gettext (String)
#else
/* No i18n for now */
#define N_(x) x
#define _(x) x
#endif
#else
/* no gettext used with gimp2 version */
#define N_(x) x
#define _(x) x
#endif

// Define location of gettext locales
// - On Win32: we use default gimp locales location, in the plugin directory:
//    ex: %appdata%\GIMP\2.99\plug-ins\fourier\locale\fr\LC_MESSAGES\gimp30-fourier.mo
// - On other platforms: we force to use the same location as GIMP application for packaging
//    ex: /usr/share/locale/fr/LC_MESSAGES/gimp30-fourier.mo
//    note: locales are not handled for user-install
#ifdef _WIN32
#else
#define GETTEXT_FORCEDIMPLOCALEDIRECTORY
#endif

/** Defines ******************************************************************/

#define PLUG_IN_BINARY "fourier"
#define PLUG_IN_NAME "plug_in_fft"
#define PLUG_IN_VERSION "Jun 2024, " VERSION

static char *PLUG_IN_AUTHOR = "Remi Peyronnet";

// Note: "known parts" should not be translated, but new parts should be (cf https://developer.gimp.org/api/3.0/libgimp/method.Procedure.add_menu_path.html)
static char *PLUG_IN_MENU_LOCATION = "<Image>/Filters/Generic";

static char *PLUG_IN_PROC = "plug-in-fourier";

static char *PLUG_IN_DIR_PROC = "plug-in-fourier-forward";
static char *PLUG_IN_DIR_MENU_LABEL = d_("FFT Forward");
static char *PLUG_IN_DIR_SHORT_DESC = d_("This plug-in applies a FFT to the image, for educational or effects purpose.");
static char *PLUG_IN_DIR_DESC = d_("Apply an FFT to the image. This can remove (for example) moire patterns from images scanned from books:\n\n" \
                                   "    The image should be RGB (Image|Mode|RGB)\n\n" \
                                   "    Remove the alpha layer, if present (Image|Flatten Image)\n\n" \
                                   "    Select Filters|Generic|FFT Forward\n\n" \
                                   "    Use the preselected neutral grey to effectively remove any moir patterns from the image. Either paint over any patterns or\n\n" \
                                   "     - In the Layers window, select the layer, and 'Duplicate Layer'\n" \
                                   "     - Select Colours|Brightness-Contrast. Increase the Contrast to see any patterns.\n" \
                                   "     - Use the Rectangular and/or Elliptical Selection tools to select any patterns on the contrast layer.\n" \
                                   "     - Then remove the contrast layer leaving the original FFT layer with the selections.\n" \
                                   "     - Then select Edit|Fill with FG colour, remembering to cancel the Selection afterwards!\n\n" \
                                   "    Select Filters|Generic|FFT Inverse\n\n" \
                                   "Voila, an image without the moire pattern!");

static char *PLUG_IN_INV_PROC = "plug-in-fourier-inverse";
static char *PLUG_IN_INV_MENU_LABEL = d_("FFT Inverse");
static char *PLUG_IN_INV_DESC = d_("Apply an inverse FFT to the image, effectively restoring the original image (plus changes).");
static char *PLUG_IN_INV_SHORT_DESC = d_("This plug-in applies a FFT to the image, for educationnal or effects purpose.");


/** Fourier Functions ===================================================== **/

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

/** Process Functions ********************************************************/

void process_fft_forward(guchar *src_pixels, guchar *dst_pixels, gint sel_width, gint sel_height, gint src_bpp, gint dst_bpp)
{

  gint row, col, row2, col2, cur_bpp, bounded, padding;
  gint progress, max_progress;
  fftw_plan p;
  double v, norm;
  double *fft_real;

  padding = (sel_width & 1) ? 1 : 2;

  fft_real = g_new(double, (sel_width + padding) * sel_height);

  progress = 0;
  max_progress = src_bpp * 3;

  p = fftw_plan_dft_r2c_2d(sel_height, sel_width, fft_real, (fftw_complex *)fft_real, FFTW_ESTIMATE);

  for (cur_bpp = 0; cur_bpp < src_bpp; cur_bpp++)
  {
    for (col = 0; col < sel_width; col++)
    {
      for (row = 0; row < sel_height; row++)
      {
        v = (double)src_pixels[(row * sel_width + col) * src_bpp + cur_bpp];
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
        dst_pixels[(row * sel_width + col) * dst_bpp + cur_bpp] = get_gchar128(col, row, bounded);
      }
    }
    // do not boost (0, 0), just offset it
    row = sel_height / 2;
    col = sel_width / 2;
    bounded = round_gint((fft_real[0] / (double)(sel_width * sel_height)) - 128.0);
    dst_pixels[(row * sel_width + col) * dst_bpp + cur_bpp] = get_gchar128(col, row, bounded);
    gimp_progress_update((double)++progress / max_progress);
  }

  fftw_destroy_plan(p);
  g_free(fft_real);
}

void process_fft_inverse(guchar *src_pixels, guchar *dst_pixels, gint sel_width, gint sel_height, gint src_bpp, gint dst_bpp)
{

  gint row, col, row2, col2, cur_bpp, bounded, padding;
  gint progress, max_progress;
  fftw_plan p;
  double v, norm;
  double *fft_real;

  padding = (sel_width & 1) ? 1 : 2;

  fft_real = g_new(double, (sel_width + padding) * sel_height);

  progress = 0;
  max_progress = src_bpp * 3;

  p = fftw_plan_dft_c2r_2d(sel_height, sel_width, (fftw_complex *)fft_real, fft_real, FFTW_ESTIMATE);

  for (cur_bpp = 0; cur_bpp < src_bpp; cur_bpp++)
  {
    for (row = 0; row < sel_height; row++)
    {
      for (col = 0; col < sel_width; col++)
      {
        map(row, col, sel_height, sel_width, &row2, &col2);
        norm = normalize(col, row, sel_width, sel_height);
        v = get_double128(row, col, src_pixels[(row * sel_width + col) * src_bpp + cur_bpp]);
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
    v = get_double128(row, col, src_pixels[(row * sel_width + col) * src_bpp + cur_bpp]);
    fft_real[0] = v + 128.0;

    gimp_progress_update((double)++progress / max_progress);
    fftw_execute(p);
    gimp_progress_update((double)++progress / max_progress);
    for (col = 0; col < sel_width; col++)
    {
      for (row = 0; row < sel_height; row++)
      {
        v = fft_real[row * (sel_width + padding) + col];
        dst_pixels[(row * sel_width + col) * dst_bpp + cur_bpp] = get_guchar(col, row, v);
      }
    }
    gimp_progress_update((double)++progress / max_progress);
  }

  fftw_destroy_plan(p);
  g_free(fft_real);
}


/** GIMP Plugin Part ====================================================== **/

#if (GIMP_MAJOR_VERSION == 3) || ((GIMP_MAJOR_VERSION == 2) && (GIMP_MINOR_VERSION >= 99))
/** GIMP 3 *******************************************************************/

// based on hot.c bundled GIMP plugin

#include <libgimp/gimpui.h>

//#define FOURIER_USE_DIALOG  false

typedef struct _Fourier Fourier;
typedef struct _FourierClass FourierClass;

struct _Fourier
{
  GimpPlugIn parent_instance;
};

struct _FourierClass
{
  GimpPlugInClass parent_class;
};

#define FOURIER_TYPE (fourier_get_type())
#define FOURIER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FOURIER_TYPE, Fourier))

#define FOURIER_DATA_DIR    (gpointer) 0x01
#define FOURIER_DATA_INV    (gpointer) 0x02

GType fourier_get_type(void) G_GNUC_CONST;

static GList *fourier_query_procedures(GimpPlugIn *plug_in);
static GimpProcedure *fourier_create_procedure(GimpPlugIn *plug_in,
                                               const gchar *name);
gboolean fourier_set_i18n ( GimpPlugIn* plug_in, const gchar* procedure_name,
                            gchar** gettext_domain, gchar** catalog_dir);

static GimpValueArray *fourier_run(GimpProcedure *procedure,
                                   GimpRunMode run_mode,
                                   GimpImage *image,
                                   GimpDrawable **drawables,
                                   GimpProcedureConfig *config,
                                   gpointer run_data);

#if FOURIER_USE_DIALOG
static gboolean plugin_dialog(GimpProcedure *procedure,
                              GObject *config);
#endif

G_DEFINE_TYPE(Fourier, fourier, GIMP_TYPE_PLUG_IN)

GIMP_MAIN(FOURIER_TYPE)

typedef enum
{
  MODE_FORWARD,
  MODE_INVERSE
} fourierModes;

static void
fourier_class_init(FourierClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(klass);

  plug_in_class->query_procedures = fourier_query_procedures;
  plug_in_class->create_procedure = fourier_create_procedure;
  plug_in_class->set_i18n = fourier_set_i18n;
}

static void
fourier_init(Fourier *fourier)
{
}

// Override standard i18n to specialize
gboolean fourier_set_i18n (
  GimpPlugIn* plug_in,
  const gchar* procedure_name,
  gchar** gettext_domain,
  gchar** catalog_dir
)
{
  *gettext_domain = g_strdup(GETTEXT_PACKAGE3);
#ifdef GETTEXT_FORCEDIMPLOCALEDIRECTORY
  *catalog_dir = g_strdup(gimp_locale_directory());
#endif
  return TRUE;
}

/*
// This was the previous code before using set_i18n function, to be included in query & run
#ifdef HAVE_GETTEXT
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE3, gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset (GETTEXT_PACKAGE3, "UTF-8");
#endif
    textdomain (GETTEXT_PACKAGE3);
#endif
*/

static GList *
fourier_query_procedures(GimpPlugIn *plug_in)
{
#if FOURIER_USE_DIALOG
  // If using dialog, we define only one procedure
  return g_list_append(NULL, g_strdup(PLUG_IN_PROC));
#else
  // If not using dialog, we define all procedures
  return g_list_append(
            g_list_append(NULL, g_strdup(PLUG_IN_DIR_PROC)),
            g_strdup(PLUG_IN_INV_PROC)
         );
#endif
}

static GimpProcedure *
fourier_create_procedure(GimpPlugIn *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;



#if FOURIER_USE_DIALOG
  if (!strcmp(name, PLUG_IN_PROC))
  {
    // One for all procedure with dialog
    procedure = gimp_image_procedure_new(plug_in, name,
                                         GIMP_PDB_PROC_TYPE_PLUGIN,
                                         fourier_run, NULL, NULL);

    gimp_procedure_set_image_types(procedure, "RGB");
    gimp_procedure_set_sensitivity_mask(procedure,
                                        GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

    gimp_procedure_set_menu_label(procedure, _("_Fourier..."));
    gimp_procedure_add_menu_path(procedure, PLUG_IN_MENU_LOCATION);

    gimp_procedure_set_documentation(procedure,
    /* menu entry short one-liner */ _(PLUG_IN_DIR_SHORT_DESC),
    /* detailed help description */  _(PLUG_IN_DIR_DESC),
                                     name);
    gimp_procedure_set_attribution(procedure,
    /* GIMP3 plugin author(s) */   PLUG_IN_AUTHOR,
    /* plugin copyright license */ "GPL3+",
    /* date(s) created/made */     PLUG_IN_VERSION);

    gimp_procedure_add_int_argument(procedure, "mode",
                                    _("Mode"),
                                    _("Mode { Foward (0), Inversed (1) }"),
                                    0, 1, MODE_FORWARD,
                                    G_PARAM_READWRITE);

    gimp_procedure_add_boolean_argument(procedure, "new-layer",
                                        _("Create _new layer"),
                                        _("Create a new layer"),
                                        TRUE,
                                        G_PARAM_READWRITE);
  }
#endif

  if (!strcmp(name, PLUG_IN_DIR_PROC))
  {
    // Forward without dialog
    procedure = gimp_image_procedure_new(plug_in, name,
                                         GIMP_PDB_PROC_TYPE_PLUGIN,
                                         fourier_run, FOURIER_DATA_DIR, NULL);

    gimp_procedure_set_image_types(procedure, "RGB");
    gimp_procedure_set_sensitivity_mask(procedure,
                                        GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

    gimp_procedure_set_menu_label(procedure, _(PLUG_IN_DIR_MENU_LABEL));
    gimp_procedure_add_menu_path(procedure, PLUG_IN_MENU_LOCATION);

    gimp_procedure_set_documentation(procedure,
                                     _(PLUG_IN_DIR_SHORT_DESC),
                                     _(PLUG_IN_DIR_DESC),
                                     name);
    gimp_procedure_set_attribution(procedure,
                                   PLUG_IN_AUTHOR,
                                   "GPL3+",
                                   PLUG_IN_VERSION);
  }
  else if (!strcmp(name, PLUG_IN_INV_PROC))
  {
    // Inverse without dialog
    procedure = gimp_image_procedure_new(plug_in, name,
                                         GIMP_PDB_PROC_TYPE_PLUGIN,
                                         fourier_run, FOURIER_DATA_INV, NULL);

    gimp_procedure_set_image_types(procedure, "RGB");
    gimp_procedure_set_sensitivity_mask(procedure,
                                        GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

    gimp_procedure_set_menu_label(procedure, _(PLUG_IN_INV_MENU_LABEL));
    gimp_procedure_add_menu_path(procedure, PLUG_IN_MENU_LOCATION);

    gimp_procedure_set_documentation(procedure,
                                     _(PLUG_IN_INV_SHORT_DESC),
                                     _(PLUG_IN_INV_DESC),
                                     name);
    gimp_procedure_set_attribution(procedure,
                                   PLUG_IN_AUTHOR,
                                   "GPL3+",
                                   PLUG_IN_VERSION);

  }

  return procedure;
}


static gboolean
fourier_core(GimpDrawable *drawable, gboolean inverse /*, gboolean new_layer*/)
{
  gint action;
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *src_format;
  const Babl *dest_format;
  gint src_bpp;
  gint dest_bpp;
  gboolean success = TRUE;
  //GimpLayer *nl = NULL;
  gint width, height;
  gint sel_x1, sel_x2, sel_y1, sel_y2;
  guchar *src, *dst;

  width = gimp_drawable_get_width(drawable);
  height = gimp_drawable_get_height(drawable);

  if (gimp_drawable_has_alpha(drawable))
    src_format = babl_format("R'G'B'A u8");
  else
    src_format = babl_format("R'G'B' u8");

  dest_format = src_format;

  /*
  if (new_layer)
  {
    gchar name[40];
    const gchar *mode_names[] =
        {
            "forward",
            "inversed",
        };

    g_snprintf(name, sizeof(name), "fourier mask (%s)", mode_names[(inverse)?1:0]);

    nl = gimp_layer_new(image, name, width, height,
                        GIMP_RGBA_IMAGE,
                        100,
                        gimp_image_get_default_new_layer_mode(image));

    gimp_drawable_fill(GIMP_DRAWABLE(nl), GIMP_FILL_TRANSPARENT);
    gimp_image_insert_layer(image, nl, NULL, 0);

    dest_format = babl_format("R'G'B'A u8");
  }
  */

  if (!gimp_drawable_mask_intersect(drawable,
                                    &sel_x1, &sel_y1, &width, &height))
    return success;

  src_bpp = babl_format_get_bytes_per_pixel(src_format);
  dest_bpp = babl_format_get_bytes_per_pixel(dest_format);

  sel_x2 = sel_x1 + width;
  sel_y2 = sel_y1 + height;

  src = g_new(guchar, width * height * src_bpp);
  dst = g_new(guchar, width * height * dest_bpp);

  src_buffer = gimp_drawable_get_buffer(drawable);

  /*if (new_layer)
  {
    dest_buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(nl));
  }
  else
  {*/
    dest_buffer = gimp_drawable_get_shadow_buffer(drawable);
  /*}*/

  gegl_buffer_get(src_buffer,
                  GEGL_RECTANGLE(sel_x1, sel_y1, width, height), 1.0,
                  src_format, src,
                  GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gimp_progress_init(inverse ? _("Applying inverse Fourier transform...") : _("Applying forward Fourier transform..."));

  if (!inverse)
  { // Forward
    process_fft_forward(src, dst, width, height, src_bpp, dest_bpp);
  }
  else
  { // Inverse
    process_fft_inverse(src, dst, width, height, src_bpp, dest_bpp);
  }

  gegl_buffer_set(dest_buffer,
                  GEGL_RECTANGLE(sel_x1, sel_y1, width, height), 0,
                  dest_format, dst,
                  GEGL_AUTO_ROWSTRIDE);

  gimp_progress_update(1.0);

  g_free(src);
  g_free(dst);

  g_object_unref(src_buffer);
  g_object_unref(dest_buffer);

  /*if (new_layer)
  {
    gimp_drawable_update(GIMP_DRAWABLE(nl), sel_x1, sel_y1, width, height);
  }
  else
  {*/
    gimp_drawable_merge_shadow(drawable, TRUE);
    gimp_drawable_update(drawable, sel_x1, sel_y1, width, height);
  /*}*/

  gimp_displays_flush();

  return success;
}

static GimpValueArray *
fourier_run(GimpProcedure *procedure,
            GimpRunMode run_mode,
            GimpImage *image,
            GimpDrawable **drawables,
            GimpProcedureConfig *config,
            gpointer run_data)
{
  GimpDrawable *drawable;
  gboolean inverse = FALSE;
  gboolean new_layer = FALSE;

  gegl_init(NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
  {
    GError *error = NULL;

    g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
                _("Procedure '%s' only works with one drawable."),
                gimp_procedure_get_name(procedure));

    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_CALLING_ERROR,
                                            error);
  }
  else
  {
    drawable = drawables[0];
  }

#if FOURIER_USE_DIALOG
  if (run_mode == GIMP_RUN_INTERACTIVE && !plugin_dialog(procedure, G_OBJECT(config)))
    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_CANCEL,
                                            NULL);

  /*g_object_get(config,
               "mode", &mode,
               "new-layer", &new_layer,
               NULL);*/
#else
  inverse = run_data == FOURIER_DATA_INV;
  new_layer = FALSE;
#endif

  if (!fourier_core(drawable, inverse /*, new_layer*/))
    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_EXECUTION_ERROR,
                                            NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();

  return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
}

#if FOURIER_USE_DIALOG

static gboolean
plugin_dialog(GimpProcedure *procedure,
              GObject *config)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkListStore *store;
  gboolean run;

  gimp_ui_init(PLUG_IN_BINARY);

  dlg = gimp_procedure_dialog_new(procedure,
                                  GIMP_PROCEDURE_CONFIG(config),
                                  _("Fourier"));

  gimp_dialog_set_alternative_button_order(GTK_DIALOG(dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient(GTK_WINDOW(dlg));

  store = gimp_int_store_new(_("_Forward"), MODE_FORWARD,
                             _("_Inverse"), MODE_INVERSE,
                             NULL);
  gimp_procedure_dialog_get_int_radio(GIMP_PROCEDURE_DIALOG(dlg),
                                      "mode", GIMP_INT_STORE(store));

  vbox = gimp_procedure_dialog_fill_box(GIMP_PROCEDURE_DIALOG(dlg),
                                        "fourier-left-side",
                                        "mode",
                                        "new-layer",
                                        NULL);
  gtk_box_set_spacing(GTK_BOX(vbox), 12);

  hbox = gimp_procedure_dialog_fill_box(GIMP_PROCEDURE_DIALOG(dlg),
                                        "fourier-hbox",
                                        "fourier-left-side",
                                        "action",
                                        NULL);
  gtk_box_set_spacing(GTK_BOX(hbox), 12);
  gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
  gtk_widget_set_margin_bottom(hbox, 12);
  gtk_orientable_set_orientation(GTK_ORIENTABLE(hbox),
                                 GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill(GIMP_PROCEDURE_DIALOG(dlg),
                             "fourier-hbox",
                             NULL);

  gtk_widget_show(dlg);

  run = gimp_procedure_dialog_run(GIMP_PROCEDURE_DIALOG(dlg));

  gtk_widget_destroy(dlg);

  return run;
}

#endif

#elif GIMP_MAJOR_VERSION == 2
/** GIMP 2 *******************************************************************/


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
      PLUG_IN_DIR_PROC,
      PLUG_IN_DIR_DESC,
      PLUG_IN_DIR_SHORT_DESC,
      PLUG_IN_AUTHOR,
      PLUG_IN_AUTHOR,
      PLUG_IN_VERSION,
      PLUG_IN_DIR_MENU_LABEL,
      "RGB*, GRAY*",
      GIMP_PLUGIN,
      G_N_ELEMENTS(args), 0,
      args, NULL);
  gimp_plugin_menu_register(PLUG_IN_DIR_PROC, PLUG_IN_MENU_LOCATION);

  /* Inverse FFT */
  gimp_install_procedure(
      PLUG_IN_INV_PROC,
      PLUG_IN_INV_DESC,
      PLUG_IN_INV_SHORT_DESC,
      PLUG_IN_AUTHOR,
      PLUG_IN_AUTHOR,
      PLUG_IN_VERSION,
      PLUG_IN_INV_MENU_LABEL,
      "RGB*, GRAY*",
      GIMP_PLUGIN,
      G_N_ELEMENTS(args), 0,
      args, NULL);
  gimp_plugin_menu_register(PLUG_IN_INV_PROC, PLUG_IN_MENU_LOCATION);
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

  if (strcmp(name, PLUG_IN_INV_PROC) == 0)
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
    gimp_progress_init(fft_inv ? _("Applying inverse Fourier transform...") : _("Applying forward Fourier transform..."));

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

    gimp_progress_init(fft_inv ? _("Inverse Fourier transform applied successfully.") : _("Forward Fourier transform applied successfully."));

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;
  }
}

#else
#error "Unsupported GIMP version"
#endif

