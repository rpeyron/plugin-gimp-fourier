// based on hot.c bundled GIMP plugin

#include <libgimp/gimpui.h>

#define GETTEXT_PACKAGE "glib"

#include "libgimp/stdplugins-intl.h"

#define FOURIER_USE_DIALOG  false

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

static GimpValueArray *fourier_run(GimpProcedure *procedure,
                                   GimpRunMode run_mode,
                                   GimpImage *image,
                                   gint n_drawables,
                                   GimpDrawable **drawables,
                                   GimpProcedureConfig *config,
                                   gpointer run_data);

static gboolean fourier_core(GimpImage *image,
                           GimpDrawable *drawable,
                           gboolean inverse,
                           gboolean new_layer);

#if FOURIER_USE_DIALOG
static gboolean plugin_dialog(GimpProcedure *procedure,
                              GObject *config);
#endif

G_DEFINE_TYPE(Fourier, fourier, GIMP_TYPE_PLUG_IN)

GIMP_MAIN(FOURIER_TYPE)
DEFINE_STD_SET_I18N

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
  plug_in_class->set_i18n = STD_SET_I18N;
}

static void
fourier_init(Fourier *fourier)
{
}

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
                                     _(PLUG_IN_DIR_SHORT_DESC),
                                     PLUG_IN_DIR_DESC,
                                     name);
    gimp_procedure_set_attribution(procedure,
                                   PLUG_IN_AUTHOR,
                                   PLUG_IN_AUTHOR,
                                   PLUG_IN_VERSION);

    GIMP_PROC_ARG_INT(procedure, "mode",
                      _("Mode"),
                      _("Mode { Foward (0), Inversed (1) }"),
                      0, 1, MODE_FORWARD,
                      G_PARAM_READWRITE);

    GIMP_PROC_ARG_BOOLEAN(procedure, "new-layer",
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
                                     PLUG_IN_DIR_DESC,
                                     name);
    gimp_procedure_set_attribution(procedure,
                                   PLUG_IN_AUTHOR,
                                   PLUG_IN_AUTHOR,
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
                                     PLUG_IN_INV_DESC,
                                     name);
    gimp_procedure_set_attribution(procedure,
                                   PLUG_IN_AUTHOR,
                                   PLUG_IN_AUTHOR,
                                   PLUG_IN_VERSION);

  }

  return procedure;
}

static GimpValueArray *
fourier_run(GimpProcedure *procedure,
            GimpRunMode run_mode,
            GimpImage *image,
            gint n_drawables,
            GimpDrawable **drawables,
            GimpProcedureConfig *config,
            gpointer run_data)
{
  GimpDrawable *drawable;
  gboolean inverse = FALSE;
  gboolean new_layer = FALSE;

  gegl_init(NULL, NULL);

  if (n_drawables != 1)
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

  g_object_get(config,
               "mode", &mode,
               "new-layer", &new_layer,
               NULL);
#else
  inverse = run_data == FOURIER_DATA_INV;
  new_layer = FALSE;
#endif                                            

  if (!fourier_core(image, drawable, inverse, new_layer))
    return gimp_procedure_new_return_values(procedure,
                                            GIMP_PDB_EXECUTION_ERROR,
                                            NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();

  return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
fourier_core(GimpImage *image, GimpDrawable *drawable, gboolean inverse, gboolean new_layer)
{
  gint action;
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *src_format;
  const Babl *dest_format;
  gint src_bpp;
  gint dest_bpp;
  gboolean success = TRUE;
  GimpLayer *nl = NULL;
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

  if (new_layer)
  {
    dest_buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(nl));
  }
  else
  {
    dest_buffer = gimp_drawable_get_shadow_buffer(drawable);
  }

  gegl_buffer_get(src_buffer,
                  GEGL_RECTANGLE(sel_x1, sel_y1, width, height), 1.0,
                  src_format, src,
                  GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gimp_progress_init(_(inverse ? "Applying inverse Fourier transform..." : "Applying forward Fourier transform..."));

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

  if (new_layer)
  {
    gimp_drawable_update(GIMP_DRAWABLE(nl), sel_x1, sel_y1, width, height);
  }
  else
  {
    gimp_drawable_merge_shadow(drawable, TRUE);
    gimp_drawable_update(drawable, sel_x1, sel_y1, width, height);
  }

  gimp_displays_flush();

  return success;
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