#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include "gui-update.h"
#include "gui.h"
#include "gui-tuner.h"
#include "tuner.h"
#include "pattern.h"
#include "settings.h"
#include "sig.h"
#include "scan.h"
#include "stationlist.h"
#include "version.h"
#include "log.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

static const gchar* const pty_list[][32] =
{
    { "None", "News", "Affairs", "Info", "Sport", "Educate", "Drama", "Culture", "Science", "Varied", "Pop M", "Rock M", "Easy M", "Light M", "Classics", "Other M", "Weather", "Finance", "Children", "Social", "Religion", "Phone In", "Travel", "Leisure", "Jazz", "Country", "Nation M", "Oldies", "Folk M", "Document", "TEST", "Alarm !" },
    { "None", "News", "Inform", "Sports", "Talk", "Rock", "Cls Rock", "Adlt Hit", "Soft Rck", "Top 40", "Country", "Oldies", "Soft", "Nostalga", "Jazz", "Classicl", "R & B", "Soft R&B", "Language", "Rel Musc", "Rel Talk", "Persnlty", "Public", "College", "N/A", "N/A", "N/A", "N/A", "N/A", "Weather", "Test", "ALERT!" }
};

static const gchar* const ecc_list[][16] =
{
    {"??", "DE", "DZ", "AD", "IL", "IT", "BE", "RU", "PS", "AL", "AT", "HU", "MT", "DE", "??", "EG" },
    {"??", "GR", "CY", "SM", "CH", "JO", "FI", "LU", "BG", "DK", "GI", "IQ", "GB", "LY", "RO", "FR" },
    {"??", "MA", "CZ", "PL", "VA", "SK", "SY", "TN", "??", "LI", "IS", "MC", "LT", "YU", "ES", "NO" },
    {"??", "??", "IE", "TR", "MK", "??", "??", "??", "NL", "LV", "LB", "??", "HR", "??", "SE", "BY" },
    {"??", "MD", "EE", "??", "??", "??", "UA", "??", "PT", "SI", "??", "??", "??", "??", "??", "BA" },
};

gboolean gui_update_freq(gpointer data)
{
    gchar buffer[8];
    g_snprintf(buffer, sizeof(buffer), "%.3f", GPOINTER_TO_INT(data)/1000.0);
    gtk_label_set_text(GTK_LABEL(gui.l_freq), buffer);
    stationlist_freq(GPOINTER_TO_INT(data));
    s.max = 0;
    s.samples = 0;
    s.sum = 0;
    g_idle_add(gui_clear_rds, NULL);
    log_cleanup();
    if(conf.grab_focus)
    {
        gui_activate();
    }
    if(conf.graph_mode == GRAPH_RESET)
    {
        signal_clear();
    }
    else if(conf.graph_mode == GRAPH_SEPARATOR)
    {
        signal_separator();
    }
    return FALSE;
}

gboolean gui_update_signal(gpointer data)
{
    s_data_t* signal = (s_data_t*)data;
    signal_push(signal->value, signal->stereo, signal->rds);

    scan_check_finished();

    if(s.stereo != signal->stereo)
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, signal->stereo?&gui.colors.stereo:&gui.colors.grey);
        s.stereo = !s.stereo;
    }

    if(s.rds != signal->rds)
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, signal->rds?&conf.color_rds:&gui.colors.grey);
        s.rds = !s.rds;
    }

    if(signal->value != lround(signal_get()->value))
    {
        stationlist_rcvlevel(lround(signal->value));
    }

    if((s.pos % 3) == 1) // slower signal level label refresh (~5 fps)
    {
        gchar *str, *s_m, *s_m2;
        if(tuner.mode == MODE_FM)
        {
            switch(conf.signal_unit)
            {
            case UNIT_DBM:
                str = g_markup_printf_escaped("<span color=\"#777777\">%4.0f%s</span>%4.0fdBm",
                                              signal_level(s.max),
                                              ((fabs(conf.signal_offset) < 0.1) ? "↑" : "↥"),
                                              signal_level(signal->value));
                break;

            case UNIT_DBUV:
                str = g_markup_printf_escaped("<span color=\"#777777\">%3.0f%s</span>%3.0f dBµV",
                                              signal_level(s.max),
                                              ((fabs(conf.signal_offset) < 0.1) ? "↑" : "↥"),
                                              signal_level(signal->value));
                break;

            case UNIT_S:
                s_m = s_meter(signal_level(s.max));
                s_m2 = s_meter(signal_level(signal->value));
                str = g_markup_printf_escaped("<span color=\"#777777\">%5s%s</span>%5s",
                                              s_m,
                                              ((fabs(conf.signal_offset) < 0.1) ? "↑" : "↥"),
                                              s_m2);
                g_free(s_m);
                g_free(s_m2);
                break;

            case UNIT_DBF:
            default:
                str = g_markup_printf_escaped("<span color=\"#777777\">%3.0f%s</span>%3.0f dBf",
                                              signal_level(s.max),
                                              ((fabs(conf.signal_offset) < 0.1) ? "↑" : "↥"),
                                              signal_level(signal->value));
                break;
            }
        }
        else
        {
            str = g_markup_printf_escaped("<span color=\"#777777\">%3.0f%s</span> %3.0f",
                                          signal_level(s.max),
                                          ((fabs(conf.signal_offset) < 0.1) ? "↑" : "↥"),
                                          signal_level(signal->value));
        }
        gtk_label_set_markup(GTK_LABEL(gui.l_sig), str);
        g_free(str);
    }

    if(conf.signal_display == SIGNAL_GRAPH)
    {
        gtk_widget_queue_draw(gui.graph);
    }
    else if(conf.signal_display == SIGNAL_BAR)
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui.p_signal), ((signal->value >= 80)? 1.0 : signal->value/80.0));
    }
    if(pattern.active)
    {
        pattern_push(signal->value);
    }
    g_free(data);
    return FALSE;
}

gboolean gui_update_pi(gpointer data)
{
    gint pi = GPOINTER_TO_INT(data) & 0xFFFF;
    gboolean unsure = (GPOINTER_TO_INT(data) & 0x10000);
    gchar buffer[6];

    g_snprintf(buffer, sizeof(buffer),
               "%04X%s",
               pi,
               (unsure ? "?" : ""));
    gtk_label_set_text(GTK_LABEL(gui.l_pi), buffer);

    stationlist_pi(pi);
    log_pi(pi, unsure);
    return FALSE;
}

gboolean gui_update_af(gpointer data)
{
    GtkTreeIter iter;
    // if new frequency is found on the AF list, gui_af_check() will set the pointer to NULL
    gtk_tree_model_foreach(GTK_TREE_MODEL(gui.af), (GtkTreeModelForeachFunc)gui_update_af_check, &data);
    if(data)
    {
        gtk_list_store_append(gui.af, &iter);
        gchar* af_new_freq = g_strdup_printf("%.1f", ((87500+GPOINTER_TO_INT(data)*100)/1000.0));
        gtk_list_store_set(gui.af, &iter, 0, GPOINTER_TO_INT(data), 1, af_new_freq, -1);
        stationlist_af(GPOINTER_TO_INT(data));
        log_af(af_new_freq);
        g_free(af_new_freq);
    }
    return FALSE;
}

gboolean gui_update_af_check(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer* newfreq)
{
    gint cfreq;
    gtk_tree_model_get(model, iter, 0, &cfreq, -1);
    if(cfreq == GPOINTER_TO_INT(*newfreq))
    {
        *newfreq = NULL;
        return TRUE; // frequency is already on the list, stop searching.
    }
    return FALSE;
}

gboolean gui_update_ps(gpointer nothing)
{
    guchar c[8];
    gint i;
    gchar *markup;
    for(i=0; i<8; i++)
    {
        c[i] = (tuner.ps_err[i] ? 110+(tuner.ps_err[i] * 12) : 0);
    }
    markup = g_markup_printf_escaped(
                 "<span color=\"#C8C8C8\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#C8C8C8\">%c</span>",
                 (conf.rds_ps_progressive?'(':'['),
                 c[0], c[0], c[0], tuner.ps[0],
                 c[1], c[1], c[1], tuner.ps[1],
                 c[2], c[2], c[2], tuner.ps[2],
                 c[3], c[3], c[3], tuner.ps[3],
                 c[4], c[4], c[4], tuner.ps[4],
                 c[5], c[5], c[5], tuner.ps[5],
                 c[6], c[6], c[6], tuner.ps[6],
                 c[7], c[7], c[7], tuner.ps[7],
                 (conf.rds_ps_progressive?')':']')
             );
    gtk_label_set_markup(GTK_LABEL(gui.l_ps), markup);
    g_free(markup);
    stationlist_ps(tuner.ps);
    log_ps(tuner.ps, tuner.ps_err);
    return FALSE;
}

gboolean gui_update_rt(gpointer flag)
{
    gchar *markup = g_markup_printf_escaped("<span color=\"#C8C8C8\">[</span>%s<span color=\"#C8C8C8\">]</span>", tuner.rt[GPOINTER_TO_INT(flag)]);
    gtk_label_set_markup(GTK_LABEL(gui.l_rt[GPOINTER_TO_INT(flag)]), markup);
    g_free(markup);
    stationlist_rt(GPOINTER_TO_INT(flag), tuner.rt[GPOINTER_TO_INT(flag)]);
    log_rt(GPOINTER_TO_INT(flag), tuner.rt[GPOINTER_TO_INT(flag)]);
    return FALSE;
}

gboolean gui_update_tp(gpointer data)
{
    gtk_label_set_text(GTK_LABEL(gui.l_tp), "TP");
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_tp), GTK_STATE_NORMAL, (GPOINTER_TO_INT(data)?&gui.colors.black:&gui.colors.grey));
    return FALSE;
}

gboolean gui_update_ta(gpointer data)
{
    gtk_label_set_text(GTK_LABEL(gui.l_ta), "TA");
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_ta), GTK_STATE_NORMAL, (GPOINTER_TO_INT(data)?&gui.colors.black:&gui.colors.grey));
    return FALSE;
}

gboolean gui_update_ms(gpointer data)
{
    if(GPOINTER_TO_INT(data))
    {
        gtk_label_set_markup(GTK_LABEL(gui.l_ms), "M<span color=\"#DDDDDD\">S</span>");
    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(gui.l_ms), "<span color=\"#DDDDDD\">M</span>S");
    }
    return FALSE;
}

gboolean gui_update_pty(gpointer data)
{
    gint id = GPOINTER_TO_INT(data);
    gtk_label_set_text(GTK_LABEL(gui.l_pty), pty_list[conf.rds_pty][id]);
    stationlist_pty(id);
    log_pty(pty_list[conf.rds_pty][id]);
    return FALSE;
}

gboolean gui_update_ecc(gpointer data)
{
    guint ecc = GPOINTER_TO_INT(data);
    /* ECC is currently displayed only in StationList and logs */
    if(tuner.pi >= 0 && (ecc >= 0xE0 && ecc <= 0xE4))
    {
        stationlist_ecc(ecc);
        log_ecc(ecc_list[ecc & 7][tuner.pi >> 12], ecc);
    }
    return FALSE;
}

gboolean gui_update_volume(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.volume), GINT_TO_POINTER(tuner_set_volume), NULL);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gui.volume), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.volume), GINT_TO_POINTER(tuner_set_volume), NULL);
    return FALSE;
}

gboolean gui_update_squelch(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.squelch), GINT_TO_POINTER(tuner_set_squelch), NULL);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gui.squelch), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.squelch), GINT_TO_POINTER(tuner_set_squelch), NULL);
    return FALSE;
}

gboolean gui_update_agc(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_agc), GINT_TO_POINTER(tuner_set_agc), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_agc), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_agc), GINT_TO_POINTER(tuner_set_agc), NULL);
    return FALSE;
}

gboolean gui_update_deemphasis(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_deemph), GINT_TO_POINTER(tuner_set_deemphasis), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_deemph), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_deemph), GINT_TO_POINTER(tuner_set_deemphasis), NULL);
    return FALSE;
}

gboolean gui_update_ant(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_ant), GINT_TO_POINTER(tuner_set_antenna), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_ant), GINT_TO_POINTER(tuner_set_antenna), NULL);
    return FALSE;
}

gboolean gui_update_gain(gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), (GPOINTER_TO_INT(data) == 10 || GPOINTER_TO_INT(data) == 11));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), (GPOINTER_TO_INT(data) ==  1 || GPOINTER_TO_INT(data) == 11));
    return FALSE;
}

gboolean gui_update_filter(gpointer data)
{
    gint i;
    for(i=0; i<FILTERS_N; i++)
    {
        if(filters[i] == GPOINTER_TO_INT(data))
        {
            g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
            gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), i);
            g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
            tuner.filter = i;
            stationlist_bw(i);
            break;
        }
    }
    return FALSE;
}

gboolean gui_update_rotator(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
    switch(GPOINTER_TO_INT(data))
    {
    case 1:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);
        break;

    case 2:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), TRUE);
        break;

    default:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);
        break;
    }
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
    return FALSE;
}

gboolean gui_update_alignment(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.adj_align), GINT_TO_POINTER(tuner_set_alignment), NULL);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(gui.adj_align), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.adj_align), GINT_TO_POINTER(tuner_set_alignment), NULL);
    return FALSE;
}

gboolean gui_clear_power_off()
{
    gui_clear();
    signal_clear();
    if(conf.signal_display == SIGNAL_GRAPH)
    {
        gtk_widget_queue_draw(gui.graph);
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui.p_signal), 0);

    gtk_widget_set_sensitive(gui.b_connect, TRUE);
    connect_button(FALSE);
    gtk_window_set_title(GTK_WINDOW(gui.window), APP_NAME);
    return FALSE;
}

gboolean gui_update_pilot(gpointer data)
{
    GtkWidget* dialog;
    gchar *msg;
    gint value = GPOINTER_TO_INT(data);

    if(value)
    {
        msg = g_markup_printf_escaped("Estimated injection level:\n<b>%.1f kHz (%0.1f%%)</b>", value/10.0, value/10.0/75.0*100);
    }
    else
    {
        msg = g_markup_printf_escaped("The stereo subcarrier is not present or the injection level is too low.");
    }
    dialog = gtk_message_dialog_new(GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), "19kHz stereo pilot");
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);
    g_free(msg);

#ifdef G_OS_WIN32
    win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    gtk_widget_destroy(dialog);
    return FALSE;
}

gboolean gui_external_event(gpointer data)
{
    switch(conf.event_action)
    {
        case ACTION_NONE:
            break;
        case ACTION_ACTIVATE:
            gui_activate();
            break;
        case ACTION_SCREENSHOT:
            gui_screenshot();
            break;
    }
    return FALSE;
}
