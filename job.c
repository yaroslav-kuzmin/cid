/*****************************************************************************/
/*                                                                           */
/* УПО - Управление Промышленым Оборудованием                                */
/*  Авторское Право (C) <2015> <Кузьмин Ярослав>                             */
/*                                                                           */
/*  Эта программа является свободным программным обеспечением:               */
/*  вы можете распространять и/или изменять это в соответствии с             */
/*  условиями в GNU General Public License, опубликованной                   */
/*  Фондом свободного программного обеспечения, как версии 3 лицензии,       */
/*  или (по вашему выбору) любой более поздней версии.                       */
/*                                                                           */
/*  Эта программа распространяется в надежде, что она будет полезной,        */
/*  но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ; даже без подразумеваемой гарантии            */
/*  Или ПРИГОДНОСТИ ДЛЯ КОНКРЕТНЫХ ЦЕЛЕЙ. См GNU General Public License      */
/*  для более подробной информации.                                          */
/*                                                                           */
/*  Вы должны были получить копию GNU General Public License                 */
/*  вместе с этой программой. Если нет, см <http://www.gnu.org/licenses/>    */
/*                                                                           */
/*  Адрес для контактов: kuzmin.yaroslav@gmail.com                           */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/* cid - control industrial device                                           */
/*  Copyright (C) <2015> <Kuzmin Yaroslav>                                   */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*  Email contact: kuzmin.yaroslav@gmail.com                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
#include <gtk/gtk.h>
#include <sqlite3.h>

#include "total.h"
#include "cid.h"

GtkWidget * load_window = NULL;
/*****************************************************************************/
void load_job(GtkButton * b,gpointer d)
{
	g_message("load_job");
	gtk_widget_destroy(load_window);
}

void load_window_destroy(GtkWidget * w,gpointer d)
{
	g_message("load_window destroy");
}
/*****************************************************************************/
char STR_LOAD_WINDOW[] = "Загрузить работу";

void create_window_load_job(GtkMenuItem * b,gpointer d)
{
	GtkWidget * hbox;
	GtkWidget * b_exit;
	load_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_screen(GTK_WINDOW (load_window),gtk_widget_get_screen(main_window));
	gtk_window_set_title(GTK_WINDOW(load_window),STR_LOAD_WINDOW);
	gtk_container_set_border_width(GTK_CONTAINER(load_window),5);

	g_signal_connect (load_window,"destroy",G_CALLBACK(load_window_destroy),NULL);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	gtk_container_add(GTK_CONTAINER(load_window),hbox);

	b_exit = gtk_button_new_with_label("Загрузить");
	g_signal_connect(b_exit,"clicked",G_CALLBACK(load_job),NULL);

	gtk_box_pack_start(GTK_BOX(hbox),b_exit,TRUE,TRUE,5);
	gtk_widget_show(b_exit);
	gtk_widget_show(hbox);
	gtk_widget_show(load_window);
}

void create_job(GtkMenuItem * b,gpointer d)
{
	g_message("Cоздать работу");
}

char STR_JOB[] = "Работа";
char STR_LOAD_JOB[] = "Загрузить";
char STR_CREATE_JOB[] = "Создать";
char STR_EXIT[] = "ВЫХОД";

GtkWidget * job_panel = NULL;

GtkWidget * create_job_panel(void)
{
	GtkWidget * menu_work;
	GtkWidget * mitem;

	job_panel = gtk_menu_bar_new();
	gtk_widget_set_halign(job_panel,GTK_ALIGN_START);

	mitem = gtk_menu_item_new_with_label(STR_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(job_panel),mitem);
	gtk_widget_show(mitem);

	menu_work = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem),menu_work);

	mitem = gtk_menu_item_new_with_label(STR_LOAD_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	g_signal_connect(mitem,"activate",G_CALLBACK(create_window_load_job),NULL);
	gtk_widget_add_accelerator(mitem,"activate",accel_group
	                          ,'L',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_widget_show(mitem);

	mitem = gtk_menu_item_new_with_label(STR_CREATE_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	g_signal_connect(mitem,"activate",G_CALLBACK(create_job),NULL);
	gtk_widget_add_accelerator(mitem,"activate",accel_group
	                          ,'C',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_widget_show(mitem);

	mitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	gtk_widget_show(mitem);

	mitem = gtk_menu_item_new_with_label(STR_EXIT);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	g_signal_connect(mitem,"activate",G_CALLBACK(main_exit),NULL);
	gtk_widget_add_accelerator(mitem,"activate",accel_group
	                          ,'Q',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_widget_show(mitem);

	mitem = create_menu_mode();
	gtk_menu_shell_append(GTK_MENU_SHELL(job_panel),mitem);

	gtk_widget_show(menu_work);
	gtk_widget_show(job_panel);
	return job_panel;
}
/*****************************************************************************/

