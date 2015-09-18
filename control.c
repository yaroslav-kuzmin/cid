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

#include <string.h>
#include <gtk/gtk.h>

#include <modbus.h>

#include "total.h"
#include "cid.h"
#include "job.h"


/*****************************************************************************/
/*  взаимодействие с контролером                                             */
/*****************************************************************************/

#define PROTOCOL_RTU     1
#define PROTOCOL_ASCII   0

static char DEFAULT_DEVICE_NAME[] = "\\\\.\\COM7";
char * device_name = NULL;
int baud = 9600;
char parity = 'N';
int data_bit = 8;
int stop_bit = 1;
int slave_id = 1;
int modbus_debug = FALSE;
int protocol = PROTOCOL_RTU;

/*****************************************************************************/

static char STR_MODBUS_GROUP[] = "modbus";
int read_config_device(void)
{
	GError * err = NULL;
	char *str = NULL;
	int value;
	/*TODO проверка вводимых данных*/
	str = g_key_file_get_string (ini_file,STR_MODBUS_GROUP,"device",&err);
	if(str == NULL){
		g_critical("Нет имени порта : %s",err->message);
		g_error_free(err);
		device_name = DEFAULT_DEVICE_NAME;
	}
	else{
		device_name = str;
	}
	g_message("Порт : %s",device_name);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_GROUP,"baud",&err);
	if(value == 0){
		g_critical("Нет скорости на порту : %s",err->message);
		g_error_free(err);
	}
	else{
		baud = value;
	}
	g_message("Скорость на порту : %d",baud);

	err = NULL;
	str = g_key_file_get_string(ini_file,STR_MODBUS_GROUP,"parity",&err);
	if(str == NULL){
		g_critical("Нет четности : %s",err->message);
		g_error_free(err);
	}
	else{
		switch(str[0]){
			case 'N':
			case 'E':
			case 'O':{
				parity = str[0];
				break;
			}
		}
	}
	g_message("Установлена четность на потру : %c",parity);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_GROUP,"data_bit",&err);
	if(value == 0){
		g_critical("Не установлено бит данных на потру: %s",err->message);
		g_error_free(err);
	}
	else{
		if((value >= 5) && (value <= 8) ){
			data_bit = value;
		}
	}
	g_message("Установлено бит данных на порту : %d",data_bit);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_GROUP,"stop_bit",&err);
	if(value == 0){
		g_critical("Не установлено стоп бита : %s",err->message);
		g_error_free(err);
	}
	else{
		if( (value >= 1) && (value <= 2)){
			stop_bit = value;
		}
	}
	g_message("Установлен стоп бит : %d",stop_bit);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_GROUP,"ID",&err);
	if(value == 0){
		g_critical("Нет номера устройства ID : %s",err->message);
		g_error_free(err);
	}
	else{
		if((value > 0) && (value < 248)){
			slave_id = value;
		}
	}

	err = NULL;
	str = g_key_file_get_string(ini_file,STR_MODBUS_GROUP,"debug",&err);
	if(str == NULL){
		g_error_free(err);
	}
	else{
		if(!g_strcmp0(str,"TRUE")){
			modbus_debug = TRUE;
		}
		else{
			modbus_debug = FALSE;
		}
	}
	g_message("Отладка протокола Modbus : %d",modbus_debug);

	err = NULL;
	str = g_key_file_get_string(ini_file,STR_MODBUS_GROUP,"protocol",&err);
	if(str == NULL){
		g_critical("Нет наименования протокола : %s",err->message);
		g_error_free(err);
	}
	else{
		if(!g_strcmp0(str,"ascii")){
			protocol = PROTOCOL_ASCII;
		}
		else{
			protocol = PROTOCOL_RTU;
		}
	}
	if(protocol == PROTOCOL_RTU){
		g_message("протокол передачи RTU");
	}
	else{
		g_message("протокол передачи ASCII");
	}
	return SUCCESS;
}

int set_status_connect(void);
int set_status_disconnect(void);

static modbus_t *ctx = NULL;

int connect_device(void)
{
	int rc = 0;

	if(ctx != NULL){
		g_critical("Устройство уже подключено");
		return CONNECT;
	}

	if(device_name == NULL){
		g_critical("Нет имени устройства");
		return DISCONNECT;
	}

	if(protocol == PROTOCOL_ASCII){
		ctx = modbus_new_ascii(device_name,baud,parity,data_bit,stop_bit);
	}else{
		ctx = modbus_new_rtu(device_name,baud,parity,data_bit,stop_bit);
	}

	if(ctx == NULL){
		g_critical("Несмог создать подключение");
	 	return DISCONNECT;
	}

	modbus_set_debug(ctx,modbus_debug);
	/*MODBUS_ERROR_RECOVERY_NONE
	  MODBUS_ERROR_RECOVERY_LINK
	  MODBUS_ERROR_RECOVERY_PROTOCOL*/
	modbus_set_error_recovery(ctx,MODBUS_ERROR_RECOVERY_NONE);

	modbus_set_slave(ctx, slave_id);
	/*TODO*/
	/*rc = modbus_connect(ctx);*/
	if(rc == -1){
		modbus_free(ctx);
		ctx = NULL;
		g_critical("Несмог подсоединится к порту : %s",device_name);
		return DISCONNECT;
	}

	set_status_connect();
	g_message("Подсоединился к порту : %s",device_name);
	return CONNECT;
}

int disconnect_device(void)
{
	if(ctx != NULL){
		/*TODO*/
		/*modbus_close(ctx);*/
		modbus_free(ctx);
		ctx = NULL;
		set_status_disconnect();
		g_message("Отсоединился от порта : %s",device_name);
	}
	return DISCONNECT;
}

int write_register(int reg,int value)
{
	int rc = 0;
	if(ctx == NULL){
		g_critical("Нет соединения с портом : %s",device_name);
		return FAILURE;
	}
	/*TODO*/
	/*rc = modbus_write_register(ctx,reg,value);*/
	if(rc == -1){
		g_critical("Несмог записать данные в порт");
		disconnect_device();
		return FAILURE;
	}
	return SUCCESS;
}

uint16_t * dest = NULL;
int amoun_dest = 127;

uint16_t * read_register(int reg,int amount)
{
	int rc = 0;
	if(amoun_dest < amount){
		g_free(dest);
		amoun_dest = amount;
		dest = g_malloc0(amoun_dest * sizeof(uint16_t));
	}
	memset(dest,0,(amoun_dest*sizeof(uint16_t)));
	if(ctx == NULL){
		g_critical("Нет соединения с портом : %s",device_name);
		return NULL;
	}
	/*TODO*/
	/*rc = modbus_read_registers(ctx,reg,amount,dest);*/
	if(rc == -1){
		g_critical("Несмог считать данные из порта");
		disconnect_device();
		return NULL;
	}
	return dest;
}

int reg_D100 = 0x1064;
int value_wait_mode = 0x00;
int value_config_mode = 0x01;
int value_auto_mode = 0x02;
int value_manual_mode = 0x03;

int set_wait_mode(void)
{
	return write_register(reg_D100,value_wait_mode);
}

int set_config_mode(void)
{
	return write_register(reg_D100,value_config_mode);
}

int set_auto_mode(void)
{
	return write_register(reg_D100,value_auto_mode);
}

int set_manual_mode(void)
{
	return write_register(reg_D100,value_manual_mode);
}

int reg_D101 = 0x1065;
int value_auto_null = 0x00;
int value_auto_start = 0x01;
int value_auto_pause = 0x02;
int value_auto_stop = 0x03;

int set_auto_null(void)
{
	return write_register(reg_D101,value_auto_null);
}

int set_auto_start(void)
{
	return write_register(reg_D101,value_auto_start);
}

int set_auto_pause(void)
{
	return write_register(reg_D101,value_auto_pause);
}

int set_auto_stop(void)
{
	return write_register(reg_D101,value_auto_stop);
}

int reg_D102 = 0x1066;
int value_manual_null = 0x00;
int value_manual_up = 0x01;
int value_manual_down = 0x02;
int value_manual_left = 0x04;
int value_manual_right = 0x08;
int value_manual_on_drive = 0x10;
int value_manual_off_drive = 0x20;

int set_manual_null(void)
{
	return write_register(reg_D102,value_manual_null);
}

int set_manual_up(void)
{
	return write_register(reg_D102,value_manual_up);
}
int set_manual_down(void)
{
	return write_register(reg_D102,value_manual_down);
}
int set_manual_left(void)
{
	return write_register(reg_D102,value_manual_left);
}
int set_manual_right(void)
{
	return write_register(reg_D102,value_manual_right);
}
int set_manual_on_drive(void)
{
	return write_register(reg_D102,value_manual_on_drive);
}
int set_manual_off_drive(void)
{
	return write_register(reg_D102,value_manual_off_drive);
}

int reg_D103 = 0x1067;

int set_uprise_angle(int value)
{
	return write_register(reg_D103,value);
}

int reg_D104 = 0x1068;

int set_lowering_angle(int value)
{
	return write_register(reg_D104,value);
}

int reg_D110 = 0x106E;
int get_angle(uint16_t * val)
{
	uint16_t * rc;
	rc = read_register(reg_D110,1);
	if(rc != NULL){
		*val = rc[0];
		return SUCCESS;
	}
	return FAILURE;
}

int reg_D111 = 0x106F;
int get_pressure(uint16_t * val)
{
	uint16_t * rc;
	rc = read_register(reg_D111,1);
	if(rc != NULL){
		*val = rc[0];
		return SUCCESS;
	}
	return FAILURE;
}

int reg_D112 = 0x1070;
int get_sensors(uint16_t * val)
{
	uint16_t * rc;
	rc = read_register(reg_D112,1);
	if(rc != NULL){
		*val = rc[0];
		return SUCCESS;
	}
	return FAILURE;
}

int reg_D113 = 0x1071;
int get_input(uint16_t * val)
{
	uint16_t * rc;
	rc = read_register(reg_D113,1);
	if(rc != NULL){
		*val = rc[0];
		return SUCCESS;
	}
	return FAILURE;
}

int reg_D114 = 0x1072;
int get_console(uint16_t * val)
{
	uint16_t * rc;
	rc = read_register(reg_D114,1);
	if(rc != NULL){
		*val = rc[0];
		return SUCCESS;
	}
	return FAILURE;
}

int set_null_mode(void)
{
	set_wait_mode();
	set_auto_null();
	set_manual_null();
	return SUCCESS;
}

int check_connect_device(void)
{
	int rc;
	uint16_t input;
	if(ctx == NULL){
		return FAILURE;
	}
	rc = get_input(&input);
	if(rc != SUCCESS){
		return FAILURE;
	}
	return SUCCESS;
}

int init_control_device(void)
{
	int rc;
	dest = g_malloc0(amoun_dest * sizeof(uint16_t));

	rc = connect_device();
	if(rc == DISCONNECT){
		return rc;
	}
	rc = check_auto_mode();
	if(rc != OK){
		/*set_null_mode();*/
	}
	return CONNECT;
}

int deinit_control_device(void)
{
	/*set_null_mode();*/
	disconnect_device();
	g_free(dest);
	return SUCCESS;
}

/*****************************************************************************/
/* панель состоянея подключения порта                                        */
/*****************************************************************************/

static GtkWidget * fra_status_connect;
static GtkWidget * lab_status;
static char STR_CONNECT[] = "Устройство подключено";
static char STR_DISCONNECT[] = "Устройство не подключено";

GtkWidget * menite_control_device;
static char STR_DEVICE[] = "Порт";
static char STR_ON_DEVICE[]  = "Включить ";
static char STR_OFF_DEVICE[] = "Выключить";

/*g_timeout_add(милисекунды,fun,userdata);*/
/* int fun (userdata)*/
/* return TRUE продолжать*/
/* return FALSE закончить*/

int check_connect_timeout(gpointer ud)
{
	check_connect_device();
	if(ctx == NULL){
		return FALSE;
	}
	return TRUE;
}

int set_status_connect(void)
{
	GdkRGBA color;
	color.red = 0;
	color.green = 1;
	color.blue = 0;
	color.alpha = 1;
	gtk_widget_override_background_color(fra_status_connect,GTK_STATE_FLAG_NORMAL,&color);
	gtk_label_set_text(GTK_LABEL(lab_status),STR_CONNECT);
	gtk_widget_override_background_color(lab_status,GTK_STATE_FLAG_NORMAL,&color);
	gtk_menu_item_set_label(GTK_MENU_ITEM(menite_control_device),STR_OFF_DEVICE);
	g_timeout_add(5000,check_connect_timeout,NULL);
	return SUCCESS;
}

int set_status_disconnect(void)
{
	GdkRGBA color;
	color.red = 1;
	color.green = 0;
	color.blue = 0;
	color.alpha = 1;
	gtk_widget_override_background_color(fra_status_connect,GTK_STATE_FLAG_NORMAL,&color);
	gtk_label_set_text(GTK_LABEL(lab_status),STR_DISCONNECT);
	gtk_widget_override_background_color(lab_status,GTK_STATE_FLAG_NORMAL,&color);
	gtk_menu_item_set_label(GTK_MENU_ITEM(menite_control_device),STR_ON_DEVICE);
	return SUCCESS;
}

GtkWidget * create_status_device(void)
{
	fra_status_connect = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(fra_status_connect),5);
	lab_status = gtk_label_new(STR_DISCONNECT);
	gtk_widget_set_hexpand(lab_status,FALSE);
	gtk_widget_set_vexpand(lab_status,FALSE);
	set_status_disconnect();
	gtk_container_add(GTK_CONTAINER(fra_status_connect),lab_status);

	gtk_widget_show(lab_status);
	gtk_widget_show(fra_status_connect);
	return fra_status_connect;
}

/*****************************************************************************/
/*  подменю включение порта                                                  */
/*****************************************************************************/

/*static char STR_SETTING_DEVICE[] = "Настройка";*/

static void activate_menu_device(GtkMenuItem * mi,gpointer ud)
{
	int rc;
	if(ctx == NULL){
		rc = init_control_device();
		if(rc != CONNECT){
			GtkWidget * md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK
			                                           ,"Несмог подключится к порту %s",device_name);
			gtk_dialog_run(GTK_DIALOG(md_err));
			gtk_widget_destroy (md_err);
			return;
		}
	}
	else{
		deinit_control_device();
	}
}
/*
static void activate_menu_setting(GtkMenuItem * mi,gpointer ud)
{
	g_message("Установил настройки видео потока");
}
*/

GtkWidget * create_menu_device(void)
{
	GtkWidget * menite_device;
	GtkWidget * men_device;
	/*GtkWidget * menite_temp;*/

	menite_device = gtk_menu_item_new_with_label(STR_DEVICE);

	men_device = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menite_device),men_device);

	menite_control_device = gtk_menu_item_new_with_label(STR_ON_DEVICE);
	g_signal_connect(menite_control_device,"activate",G_CALLBACK(activate_menu_device),NULL);
	gtk_widget_add_accelerator(menite_control_device,"activate",accgro_main
	                          ,'P',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_device),menite_control_device);
	gtk_widget_show(menite_control_device);

	/*TODO добавить настройки*/
/*
	menite_temp = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(men_device),menite_temp);
	gtk_widget_show(menite_temp);
	menite_temp = gtk_menu_item_new_with_label(STR_SETTING_DEVICE);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_setting),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'S',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_device),menite_temp);
	gtk_widget_show(menite_temp);
*/

	gtk_widget_show(menite_device);

	return menite_device;
}

/*****************************************************************************/
