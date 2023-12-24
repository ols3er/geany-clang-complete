/*
 * preferences.cpp - a Geany plugin to provide code completion using clang
 *
 * Copyright (C) 2014-2015 Noto, Yuta <nonotetau(at)gmail(dot)com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <geanycc/geanycc.hpp>
#include <geanycc/utils.hpp>

#include <vector>
#include <string>
#include <sstream>

#include "completion_framework.hpp"

#include "preferences.hpp"

// config dialog implements

#include <geanyplugin.h>

static struct PrefWidget
{
    GtkWidget* start_with_dot;
    GtkWidget* start_with_arrow;
    GtkWidget* start_with_scope_res;
    GtkWidget* row_text_max_spinbtn;
    GtkWidget* swin_height_max_spinbtn;
    GtkTextBuffer* options_text_buf;
    GtkEntryBuffer* command_buffer;

} pref_widgets;

static void on_configure_response(GtkDialog* dialog, gint response, gpointer user_data)
{
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY) {
	g_print("clang complete: modified preferences\n");
	auto self = (geanycc::CppCompletionFramework*)user_data;

	ClangCompletePluginPref* pref = ClangCompletePluginPref::instance();

	pref->compiler_options.clear();
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(pref_widgets.options_text_buf, &start);
	gtk_text_buffer_get_end_iter(pref_widgets.options_text_buf, &end);
	gchar* text = gtk_text_buffer_get_text(pref_widgets.options_text_buf, &start, &end, FALSE);
	std::istringstream iss(text);
	std::string tmp;
	while (std::getline(iss, tmp, '\n')) {
		if (tmp.length() != 0) {
			if (tmp[tmp.length() - 1] == '\r') {
				tmp = tmp.substr(0, tmp.length() - 1);
			}
			pref->compiler_options.push_back(tmp);
		}
	}
	g_free(text);

	pref->start_completion_with_dot =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.start_with_dot));
	pref->start_completion_with_arrow =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.start_with_arrow));
	pref->start_completion_with_scope_res =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widgets.start_with_scope_res));

	pref->row_text_max =
	    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pref_widgets.row_text_max_spinbtn));

	pref->suggestion_window_height_max =
	    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pref_widgets.swin_height_max_spinbtn));

	self->save_preferences();
	self->updated_preferences();
    }
}

static void on_click_exec_button(GtkButton* button, gpointer user_data)
{
    std::string output;
    static const int BUF_SIZE = 512;
    char buf[BUF_SIZE];

    const gchar* command = gtk_entry_buffer_get_text(pref_widgets.command_buffer);
    FILE* fp = popen(command, "r");
    if (fp) {
	while (fgets(buf, BUF_SIZE, fp) != NULL) {
	    output += buf;
	}
    }
    pclose(fp);

    // parse the command output
    std::vector<std::string> options;
    std::string s = "";
    int espace = false;

    for (const char* p = output.c_str(); *p != '\0'; ++p) {
	char c = *p;
	switch (c) {
	    case '\"': {
		if (espace) {
		    if (!s.empty()) {
			options.push_back(s);
			s = "";
		    }
		}
		espace = !espace;
	    } break;
	    case '\t': case ' ': case '\r': case '\n': {
		if (espace) {
		    s += c;
		} else {
		    if (!s.empty()) {
			options.push_back(s);
			s = "";
		    }
		}
	    } break;
	    default:
		s += c;
	}
    }

    std::string write_str;
    for (size_t i = 0; i < options.size(); i++) {
	write_str += options[i].c_str();
	write_str += "\n";
    }
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(pref_widgets.options_text_buf, &iter);
    gtk_text_buffer_insert(pref_widgets.options_text_buf, &iter, write_str.c_str(), -1);
}

#define GETOBJ(name) GTK_WIDGET(gtk_builder_get_object(builder, name))

GtkWidget* geanycc::CppCompletionFramework::create_config_widget(GtkDialog* dialog)
{
    g_debug("code complete: plugin_configure");
    ClangCompletePluginPref* pref = ClangCompletePluginPref::instance();

    GError* err = NULL;
    GtkBuilder* builder = gtk_builder_new();
// defined prefcpp_ui, prefcpp_ui_len
#include "data/prefcpp_ui.hpp"
    gint ret = gtk_builder_add_from_string(builder, (gchar*)prefcpp_ui, prefcpp_ui_len, &err);
    if (err) {
	printf("fail to load preference ui: %s\n", err->message);
	// GtkWidget* vbox = gtk_vbox_new(FALSE, 5);
	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	return vbox;
    }

    pref_widgets.start_with_dot = GETOBJ("cbtn_dot");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref_widgets.start_with_dot),
				 pref->start_completion_with_dot);

    pref_widgets.start_with_arrow = GETOBJ("cbtn_arrow");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref_widgets.start_with_arrow),
				 pref->start_completion_with_arrow);

    pref_widgets.start_with_scope_res = GETOBJ("cbtn_nameres");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref_widgets.start_with_scope_res),
				 pref->start_completion_with_scope_res);

    // compiler options
    GtkWidget* options_text_view = GETOBJ("tv_compileopt");
    // load compiler options
    pref_widgets.options_text_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(options_text_view));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(pref_widgets.options_text_buf, &iter, 0);
    for (size_t i = 0; i < pref->compiler_options.size(); i++) {
	    gtk_text_buffer_insert(pref_widgets.options_text_buf, &iter,
				   pref->compiler_options[i].c_str(), -1);
	    gtk_text_buffer_insert(pref_widgets.options_text_buf, &iter, "\n", -1);
    }

    // get option form command
    GtkWidget* command_entry = GETOBJ("te_comquery");
    pref_widgets.command_buffer = gtk_entry_get_buffer(GTK_ENTRY(command_entry));

    GtkWidget* command_exec_button = GETOBJ("btn_runcom");
    g_signal_connect(command_exec_button, "clicked", G_CALLBACK(on_click_exec_button), NULL);

    // ** suggestion window **
    pref_widgets.row_text_max_spinbtn = GETOBJ("spin_rowtextmax");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pref_widgets.row_text_max_spinbtn),
			      pref->row_text_max);

    pref_widgets.swin_height_max_spinbtn = GETOBJ("spin_sugwinheight");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pref_widgets.swin_height_max_spinbtn),
			      pref->suggestion_window_height_max);

    g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), this);
    GtkWidget* vbox = GETOBJ("box_prefcpp");
    return vbox;
}

void geanycc::CppCompletionFramework::load_preferences()
{
    ClangCompletePluginPref* pref = ClangCompletePluginPref::instance();

    std::string config_file = get_config_file();

    /* Initialising options from config file */
    GKeyFile* keyfile = g_key_file_new();
    if (g_key_file_load_from_file(keyfile, config_file.c_str(), G_KEY_FILE_NONE, NULL)) {
	const char* group = get_plugin_name();
	pref->start_completion_with_dot =
	    g_key_file_get_boolean(keyfile, group, "start_completion_with_dot", NULL);
	pref->start_completion_with_arrow =
	    g_key_file_get_boolean(keyfile, group, "start_completion_with_arrow", NULL);
	pref->start_completion_with_scope_res = g_key_file_get_boolean(
	    keyfile, group, "start_completion_with_scope_resolution", NULL);
	pref->row_text_max =
	    g_key_file_get_integer(keyfile, group, "maximum_char_in_row", NULL);
	pref->suggestion_window_height_max =
	    g_key_file_get_integer(keyfile, group, "maximum_sug_window_height", NULL);
	pref->compiler_options =
	    geanycc::util::get_vector_from_keyfile_stringlist(
		keyfile, group, "compiler_options", NULL);

	// group, type, key, default-value
    } else {
	pref->compiler_options.clear();
	pref->start_completion_with_dot = true;
	pref->start_completion_with_arrow = true;
	pref->start_completion_with_scope_res = true;
	pref->row_text_max = 120;
	pref->suggestion_window_height_max = 300;
    }
    g_key_file_free(keyfile);

    this->updated_preferences();
}

void geanycc::CppCompletionFramework::save_preferences()
{
    std::string config_file = get_config_file();
    GKeyFile* keyfile = g_key_file_new();
    const char* group = get_plugin_name();
    ClangCompletePluginPref* pref = ClangCompletePluginPref::instance();
    g_key_file_set_boolean(keyfile, group, "start_completion_with_dot",
			   pref->start_completion_with_dot);
    g_key_file_set_boolean(keyfile, group, "start_completion_with_arrow",
			   pref->start_completion_with_arrow);
    g_key_file_set_boolean(keyfile, group, "start_completion_with_scope_resolution",
			   pref->start_completion_with_scope_res);
    g_key_file_set_integer(keyfile, group, "maximum_char_in_row", pref->row_text_max);
    g_key_file_set_integer(keyfile, group, "maximum_sug_window_height",
			   pref->suggestion_window_height_max);
    geanycc::util::set_keyfile_stringlist_by_vector(keyfile, group, "compiler_options",
						    pref->compiler_options);

    geanycc::util::save_keyfile(keyfile, config_file.c_str());

    g_key_file_free(keyfile);
}

void geanycc::CppCompletionFramework::updated_preferences()
{
    ClangCompletePluginPref* pref = ClangCompletePluginPref::instance();
    this->set_completion_option(pref->compiler_options);
    if (this->suggestion_window) {
	this->suggestion_window->set_max_char_in_row(pref->row_text_max);
	this->suggestion_window->set_max_window_height(pref->suggestion_window_height_max);
    }
}
