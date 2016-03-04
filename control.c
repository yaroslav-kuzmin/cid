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

#define TEST_INTERFACE              FALSE

#define PROTOCOL_RTU     1
#define PROTOCOL_ASCII   0

static char DEFAULT_DEVICE_NAME[] = "\\\\.\\COM7";
static char * device_name = NULL;
static int baud = 9600;
static char parity = 'N';
static int data_bit = 8;
static int stop_bit = 1;
static int slave_id = 1;
static int modbus_debug = FALSE;
static int protocol = PROTOCOL_RTU;

/*****************************************************************************/

static int read_config_device(void)
{
	GError * err = NULL;
	char *str = NULL;
	int value;

	str = g_key_file_get_string (ini_file,STR_MODBUS_KEY,"device",&err);
	if(str == NULL){
		g_critical("Нет имени порта : %s!",err->message);
		g_error_free(err);
		device_name = DEFAULT_DEVICE_NAME;
	}
	else{
		device_name = str;
	}
	g_message("Порт : %s.",device_name);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_KEY,"baud",&err);
	if(value == 0){
		g_critical("Нет скорости на порту : %s!",err->message);
		g_error_free(err);
	}
	else{
		baud = value;
	}
	g_message("Скорость на порту : %d.",baud);

	err = NULL;
	str = g_key_file_get_string(ini_file,STR_MODBUS_KEY,"parity",&err);
	if(str == NULL){
		g_critical("Нет четности : %s!",err->message);
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
	g_message("Установлена четность на потру : %c.",parity);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_KEY,"data_bit",&err);
	if(value == 0){
		g_critical("Не установлено бит данных на потру: %s!",err->message);
		g_error_free(err);
	}
	else{
		if((value >= 5) && (value <= 8) ){
			data_bit = value;
		}
	}
	g_message("Установлено бит данных на порту : %d.",data_bit);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_KEY,"stop_bit",&err);
	if(value == 0){
		g_critical("Не установлено стоп бита : %s!",err->message);
		g_error_free(err);
	}
	else{
		if( (value >= 1) && (value <= 2)){
			stop_bit = value;
		}
	}
	g_message("Установлен стоп бит : %d.",stop_bit);

	err = NULL;
	value = g_key_file_get_integer(ini_file,STR_MODBUS_KEY,"ID",&err);
	if(value == 0){
		g_critical("Нет номера устройства ID : %s!",err->message);
		g_error_free(err);
	}
	else{
		if((value > 0) && (value < 248)){  /*параметры протокола Modbus*/
			slave_id = value;
		}
	}

	err = NULL;
	str = g_key_file_get_string(ini_file,STR_MODBUS_KEY,"debug",&err);
	if(str == NULL){
		g_error_free(err);
	}
	else{
		if(!g_strcmp0(str,"TRUE")){
			/*TODO debug only modbus_debug = TRUE;*/
			modbus_debug = FALSE;
		}
		else{
			modbus_debug = FALSE;
		}
	}
	g_message("Отладка протокола Modbus : %d.",modbus_debug);

	err = NULL;
	str = g_key_file_get_string(ini_file,STR_MODBUS_KEY,"protocol",&err);
	if(str == NULL){
		g_critical("Нет наименования протокола : %s!",err->message);
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
		g_message("протокол передачи RTU.");
	}
	else{
		g_message("протокол передачи ASCII.");
	}
	return SUCCESS;
}

static int set_status_connect(void);
static int set_status_disconnect(void);

static modbus_t *ctx = NULL;

static int connect_device(void)
{
	int rc = 0;

	if(ctx != NULL){
		g_critical("Устройство уже подключено!");
		return CONNECT;
	}

	if(device_name == NULL){
		g_critical("Нет имени устройства!");
		return DISCONNECT;
	}

	if(protocol == PROTOCOL_ASCII){
		ctx = modbus_new_ascii(device_name,baud,parity,data_bit,stop_bit);
	}else{
		ctx = modbus_new_rtu(device_name,baud,parity,data_bit,stop_bit);
	}

	if(ctx == NULL){
		g_critical("Несмог создать подключение!");
	 	return DISCONNECT;
	}

	modbus_set_debug(ctx,modbus_debug);

	/*MODBUS_ERROR_RECOVERY_NONE
	  MODBUS_ERROR_RECOVERY_LINK
	  MODBUS_ERROR_RECOVERY_PROTOCOL*/
	modbus_set_error_recovery(ctx,MODBUS_ERROR_RECOVERY_NONE);

	modbus_set_slave(ctx, slave_id);
#if !TEST_INTERFACE
	rc = modbus_connect(ctx);
#endif
	if(rc == -1){
		modbus_free(ctx);
		ctx = NULL;
		g_critical("Несмог подсоединится к порту : %s!",device_name);
		return DISCONNECT;
	}

	set_status_connect();
	g_message("Подсоединился к порту : %s.",device_name);
	return CONNECT;
}

static int disconnect_device(void)
{
	if(ctx != NULL){
#if !TEST_INTERFACE
		modbus_close(ctx);
#endif
		modbus_free(ctx);
		ctx = NULL;
		set_status_disconnect();
		g_message("Отсоединился от порта : %s.",device_name);
	}
	return DISCONNECT;
}

static int write_register(int reg,int value)
{
	int rc = 0;
	if(ctx == NULL){
		g_critical("Нет соединения с портом : %s!",device_name);
		return FAILURE;
	}
#if !TEST_INTERFACE
	rc = modbus_write_register(ctx,reg,value);
#endif
	if(rc == -1){
		g_critical("Несмог записать данные в порт!");
		disconnect_device();
		return FAILURE;
	}
	return SUCCESS;
}

static uint16_t * dest = NULL;

static const int amount_dest = (260/2) + 1;  /*максимальная длина в протоколе modbus RS232/RS485 256 байт TCP = 260 */

static uint16_t * read_register(int reg,int amount)
{
	int rc = 0;

	if(amount_dest < amount){
		return NULL;
	}
	if(ctx == NULL){
		g_critical("Нет соединения с портом : %s!",device_name);
		return NULL;
	}
#if !TEST_INTERFACE
	rc = modbus_read_registers(ctx,reg,amount,dest);
#endif
	if(rc == -1){
		g_critical("Несмог считать данные из порта!");
		disconnect_device();
		return NULL;
	}
	return dest;
}

static int reg_D100 = 0x1064;
static int value_wait_mode = 0x00;
static int value_config_mode = 0x01;
static int value_auto_mode = 0x02;
static int value_manual_mode = 0x03;

int command_wait_mode(void)
{
	return write_register(reg_D100,value_wait_mode);
}

int command_config_mode(void)
{
	return write_register(reg_D100,value_config_mode);
}

int command_auto_mode(void)
{
	return write_register(reg_D100,value_auto_mode);
}

int command_manual_mode(void)
{
	return write_register(reg_D100,value_manual_mode);
}

static int reg_D101 = 0x1065;
static int value_auto_stop = 0x00;
static int value_auto_start = 0x01;
static int value_auto_pause = 0x02;

int command_auto_null(void)
{
	return write_register(reg_D101,value_auto_stop);
}

int command_auto_start(void)
{
	return write_register(reg_D101,value_auto_start);
}

int command_auto_pause(void)
{
	return write_register(reg_D101,value_auto_pause);
}

int command_auto_stop(void)
{
	return write_register(reg_D101,value_auto_stop);
}

static int reg_D102 = 0x1066;
static uint16_t value_manual_current = 0x00;
#define MANUAL_MODE_NULL           0x0000
#define MANUAL_MODE_UP_ON          0x0001
#define MANUAL_MODE_UP_OFF         (0xFFFF^MANUAL_MODE_UP_ON)
#define MANUAL_MODE_DOWN_ON        0x0002
#define MANUAL_MODE_DOWN_OFF       (0xFFFF^MANUAL_MODE_DOWN_ON)
#define MANUAL_MODE_LEFT_ON        0x0004
#define MANUAL_MODE_LEFT_OFF       (0xFFFF^MANUAL_MODE_LEFT_ON)
#define MANUAL_MODE_RIGHT_ON       0x0008
#define MANUAL_MODE_RIGHT_OFF      (0xFFFF^MANUAL_MODE_RIGHT_ON)
#define MANUAL_MODE_DRIVE_ON       0x0010
#define MANUAL_MODE_DRIVE_OFF      (0xFFFF^MANUAL_MODE_DRIVE_ON)
#define MANUAL_MODE_LASER_ON       0x0020
#define MANUAL_MODE_LASER_OFF      (0xFFFF^MANUAL_MODE_LASER_ON)

int command_manual_null(void)
{
	value_manual_current = MANUAL_MODE_NULL;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_up_on(void)
{
	value_manual_current = value_manual_current | MANUAL_MODE_UP_ON;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_up_off(void)
{
	value_manual_current = value_manual_current & MANUAL_MODE_UP_OFF;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_down_on(void)
{
	value_manual_current = value_manual_current | MANUAL_MODE_DOWN_ON;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_down_off(void)
{
	value_manual_current = value_manual_current & MANUAL_MODE_DOWN_OFF;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_left_on(void)
{
	value_manual_current = value_manual_current | MANUAL_MODE_LEFT_ON;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_left_off(void)
{
	value_manual_current = value_manual_current & MANUAL_MODE_LEFT_OFF;
	return write_register(reg_D102,value_manual_current);
}


int command_manual_right_on(void)
{
	value_manual_current = value_manual_current | MANUAL_MODE_RIGHT_ON;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_right_off(void)
{
	value_manual_current = value_manual_current & MANUAL_MODE_RIGHT_OFF;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_drive_on(void)
{
	value_manual_current = value_manual_current | MANUAL_MODE_DRIVE_ON;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_drive_off(void)
{
	value_manual_current = value_manual_current & MANUAL_MODE_DRIVE_OFF;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_laser_on(void)
{
	value_manual_current = value_manual_current | MANUAL_MODE_LASER_ON;
	return write_register(reg_D102,value_manual_current);
}

int command_manual_laser_off(void)
{
	value_manual_current = value_manual_current & MANUAL_MODE_LASER_OFF;
	return write_register(reg_D102,value_manual_current);
}

static int reg_D103 = 0x1067;

int command_uprise_angle(uint16_t angle)
{
	if( (angle < MIN_ANGLE_IN_TIC) || (angle > MAX_ANGLE_IN_TIC)){
		angle = MAX_ANGLE_IN_TIC;
	}
	return write_register(reg_D103,angle);
}

static int reg_D104 = 0x1068;

int command_lowering_angle(uint16_t angle)
{
	if( (angle < MIN_ANGLE_IN_TIC) || (angle > MAX_ANGLE_IN_TIC)){
		angle = MIN_ANGLE_IN_TIC;
	}
	return write_register(reg_D104,angle);
}

static int reg_D105 = 0x1069;

int command_pressure(uint16_t pressure)
{
	if( (pressure < MIN_PRESSURE_IN_TIC) || (pressure > MAX_PRESSURE_IN_TIC)){
		pressure = DEFAULT_PRESSURE_IN_TIC;
	}
	return write_register(reg_D105,pressure);
}

static int reg_D115 = 0x1073;

int command_speed_vertical(uint16_t speed)
{
	if(speed > MAX_SPEED_VERTICAL_IN_TIC){
		speed = MAX_SPEED_VERTICAL_IN_TIC;
	}
	return write_register(reg_D115,speed);
}

static int reg_D116 = 0x1074;

int command_valve(uint16_t value)
{
	if(value > MAX_VALVE_IN_TIC){
		value = MAX_VALVE_IN_TIC;
	}
	return write_register(reg_D116,value);
}

static int reg_D117 = 0x1075;

int command_horizontal(uint16_t horizontal_offset)
{
	if(horizontal_offset > MAX_HORIZONTAL_OFFSET_IN_TIC){
		horizontal_offset = MAX_HORIZONTAL_OFFSET_IN_TIC;
	}
	return write_register(reg_D117,horizontal_offset);
}

int command_null_mode(void)
{
	command_wait_mode();
	command_auto_null();
	command_manual_null();
	command_uprise_angle(0);
	command_lowering_angle(0);
	command_pressure(0);
	command_speed_vertical(0);
	command_valve(0);
	command_pressure(0);
	command_horizontal(0);
	return SUCCESS;
}


static int reg_D110 = 0x106E;
/*static int reg_D111 = 0x106F;*/
/*static int reg_D112 = 0x1070;*/
/*static int reg_D113 = 0x1071;*/
/*static int reg_D114 = 0x1072;*/
#define AMOUNT_INFO_REGISTR    5

struct _status_console_s
{
	uint16_t up:1;
	uint16_t down:1;
	uint16_t left:1;
	uint16_t right:1;
	uint16_t pause:1;
	uint16_t stop:1;
	uint16_t on_valve:1;
	uint16_t off_valve:1;
	uint16_t reserv:8;
}__attribute__((packed));
typedef struct _status_console_s status_console_s;
struct _info_s
{
	uint16_t angle;           /*D110*/
	uint16_t pressure;        /*D111*/
	uint16_t sensors;         /*D112*/
	uint16_t input;           /*D113*/
	status_console_s console; /*D114*/
}__attribute__((packed));
typedef struct _info_s info_s;

info_s * info = NULL;

int command_info(void)
{
	uint16_t * rc = read_register(reg_D110,AMOUNT_INFO_REGISTR);
	if(rc != NULL){
		return SUCCESS;
	}
	return FAILURE;
}

uint16_t info_angle(void)
{
	return info->angle;
}
uint16_t info_pressure(void)
{
	return info->pressure;
}
uint16_t info_sensors(void)
{
	return info->sensors;
}
uint16_t info_input(void)
{
	return info->input;
}
int info_console_up(void)
{
	return info->console.up;
}
int info_console_down(void)
{
	return info->console.down;
}
int info_console_left(void)
{
	return info->console.left;
}
int info_console_right(void)
{
	return info->console.right;
}
int info_console_pause(void)
{
	return info->console.pause;
}
int info_console_stop(void)
{
	return info->console.stop;
}
int info_console_on_valve(void)
{
	return info->console.on_valve;
}
int info_console_off_valve(void)
{
	return info->console.off_valve;
}

int check_connect_device(uint16_t * status)
{
	int rc;

	if(ctx == NULL){
		return FAILURE;
	}

	rc = command_info();
	if( rc != SUCCESS){
		return FAILURE;
	}
	if(status != NULL){
		*status = info_sensors();
	}
	return SUCCESS;
}

static int init_control_device(void)
{
	int rc;
	if(dest == NULL){
		dest = g_malloc0(amount_dest * sizeof(uint16_t));
		info = (info_s*)dest;
	}
	rc = connect_device();
	if(rc == DISCONNECT){
		return rc;
	}
	rc = check_auto_mode();
	if(rc != OK){
		command_null_mode();
	}
	return CONNECT;
}

int deinit_control_device(void)
{
	int rc;
	rc = check_auto_mode();
	if(rc != OK){
		command_null_mode();
	}
	disconnect_device();
	if(dest != NULL){
		g_free(dest);
		dest = NULL;
		info = NULL;
	}
	return SUCCESS;
}

/*****************************************************************************/
/* панель состоянея подключения порта                                        */
/*****************************************************************************/

static GtkWidget * fra_status_connect;
static GtkWidget * lab_status;
static GtkWidget * lab_limit_vertical;
static GtkWidget * lab_limit_horizontal;
static GtkWidget * lab_device;
static GtkWidget * lab_pressure;

static char STR_CONNECT[] = "Установка подключена";
static char STR_DISCONNECT[] = "Установка не подключена";
static char STR_LIMIT_VERTICAL[] =        "Предел по вертикали!";
static char STR_LIMIT_VERTICAL_TOP[] =    "Предел ВЕРХ!";
static char STR_LIMIT_VERTICAL_BOTTOM[] = "Предел НИЗ!";
static char STR_LIMIT_HORIZONTAL[] =       "Предел по горизонтали!";
static char STR_LIMIT_HORIZONTAL_LEFT[] =  "Предел ЛЕВО!";
static char STR_LIMIT_HORIZONTAL_RIGHT[] = "Предел ПРАВО!";
static char STR_DEVICE_CRASH[] = "Установка - АВАРИЯ!";
static char STR_DEVICE_NORM[] =  "Установка - норма!";
static char STR_PRESSURE_CRASH[] = "Давление - АВАРИЯ!";
static char STR_PRESSURE_NORM[] = "Давление - норма!";

static GtkWidget * menite_control_device;
static char STR_DEVICE[] = "Порт";
static char STR_ON_DEVICE[]  = "Включить ";
static char STR_OFF_DEVICE[] = "Выключить";

#define DEVICE_NORM                      0x00

#define DEVICE_LIMIT_VERTICAL            0x03
#define DEVICE_LIMIT_VERTICAL_TOP        0x01
#define DEVICE_LIMIT_VERTICAL_BOTTOM     0x02

#define DEVICE_LIMIT_HORIZONTAL          0x0C
#define DEVICE_LIMIT_HORIZONTAL_LEFT     0x04
#define DEVICE_LIMIT_HORIZONTAL_RIGHT    0x08

#define DEVICE_CRASH                     0x30
#define DEVICE_CRASH_VERTICAL            0x10
#define DEVICE_CRASH_HORIZONTAL          0x20

#define DEVICE_PRESSURE                  0x40

static int set_status_limit_vertical(int status)
{
	static int old_status = DEVICE_NORM;

	if( old_status != (status & DEVICE_LIMIT_VERTICAL) ){
		old_status = (status & DEVICE_LIMIT_VERTICAL);
		switch (old_status){
			case DEVICE_LIMIT_VERTICAL_TOP:{
				gtk_label_set_text(GTK_LABEL(lab_limit_vertical),STR_LIMIT_VERTICAL_TOP);
				set_size_font(lab_limit_vertical,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_LIMIT_VERTICAL_BOTTOM:{
				gtk_label_set_text(GTK_LABEL(lab_limit_vertical),STR_LIMIT_VERTICAL_BOTTOM);
				set_size_font(lab_limit_vertical,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_LIMIT_VERTICAL:{
				gtk_label_set_text(GTK_LABEL(lab_limit_vertical),STR_LIMIT_VERTICAL);
				set_size_font(lab_limit_vertical,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_NORM:{
				gtk_label_set_text(GTK_LABEL(lab_limit_vertical),STR_LIMIT_VERTICAL);
				set_size_font(lab_limit_vertical,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_green);
				gtk_widget_override_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_black);
				break;
			}
			default:{
				gtk_label_set_text(GTK_LABEL(lab_limit_vertical),STR_LIMIT_VERTICAL);
				set_size_font(lab_limit_vertical,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_vertical,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
		}
	}
	return SUCCESS;
}

static int set_status_limit_horizontal(int status)
{
	static int old_status = DEVICE_NORM;

	if( old_status != (status & DEVICE_LIMIT_HORIZONTAL) ){
		old_status = (status & DEVICE_LIMIT_HORIZONTAL);
		switch (old_status){
			case DEVICE_LIMIT_HORIZONTAL_LEFT:{
				gtk_label_set_text(GTK_LABEL(lab_limit_horizontal),STR_LIMIT_HORIZONTAL_LEFT);
				set_size_font(lab_limit_horizontal,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_LIMIT_HORIZONTAL_RIGHT:{
				gtk_label_set_text(GTK_LABEL(lab_limit_horizontal),STR_LIMIT_HORIZONTAL_RIGHT);
				set_size_font(lab_limit_horizontal,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_LIMIT_HORIZONTAL:{
				gtk_label_set_text(GTK_LABEL(lab_limit_horizontal),STR_LIMIT_HORIZONTAL);
				set_size_font(lab_limit_horizontal,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_NORM:{
				gtk_label_set_text(GTK_LABEL(lab_limit_horizontal),STR_LIMIT_HORIZONTAL);
				set_size_font(lab_limit_horizontal,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_green);
				gtk_widget_override_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_black);
				break;
			}
			default:{
				gtk_label_set_text(GTK_LABEL(lab_limit_horizontal),STR_LIMIT_HORIZONTAL);
				set_size_font(lab_limit_horizontal,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_limit_horizontal,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
		}
	}
	return SUCCESS;
}

static int set_status_device(int status)
{
	static int old_status = DEVICE_NORM;

	if( old_status != (status & DEVICE_CRASH) ){
		old_status = (status & DEVICE_CRASH);
		switch (old_status){
			case DEVICE_CRASH_VERTICAL:
			case DEVICE_CRASH_HORIZONTAL:{
				gtk_label_set_text(GTK_LABEL(lab_device),STR_DEVICE_CRASH);
				set_size_font(lab_device,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_CRASH:{
				gtk_label_set_text(GTK_LABEL(lab_device),STR_DEVICE_NORM);
				set_size_font(lab_device,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_white);

				break;
			}
			case DEVICE_NORM:{
				gtk_label_set_text(GTK_LABEL(lab_device),STR_DEVICE_NORM);
				set_size_font(lab_device,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_green);
				gtk_widget_override_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_black);
				break;
			}
			default:{
				gtk_label_set_text(GTK_LABEL(lab_device),STR_DEVICE_NORM);
				set_size_font(lab_device,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_device,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
		}
	}
	return SUCCESS;
}

static int set_status_pressure(int status)
{
	static int old_status = DEVICE_NORM;

	if(old_status != (status & DEVICE_PRESSURE)){
		old_status = (status & DEVICE_PRESSURE);
		switch(old_status){
			case DEVICE_PRESSURE:{
				gtk_label_set_text(GTK_LABEL(lab_pressure),STR_PRESSURE_CRASH);
				set_size_font(lab_pressure,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_pressure,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_pressure,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
			case DEVICE_NORM:{
				gtk_label_set_text(GTK_LABEL(lab_pressure),STR_PRESSURE_NORM);
				set_size_font(lab_pressure,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_pressure,GTK_STATE_FLAG_NORMAL,&color_green);
				gtk_widget_override_color(lab_pressure,GTK_STATE_FLAG_NORMAL,&color_black);
				break;
			}
			default:{
				gtk_label_set_text(GTK_LABEL(lab_pressure),STR_PRESSURE_NORM);
				set_size_font(lab_pressure,SIZE_FONT_STATUS_DEVICE);
				gtk_widget_override_background_color(lab_pressure,GTK_STATE_FLAG_NORMAL,&color_red);
				gtk_widget_override_color(lab_pressure,GTK_STATE_FLAG_NORMAL,&color_white);
				break;
			}
		}
	}
	return SUCCESS;
}

static int check_connect_timeout(gpointer ud)
{
	uint16_t status_sensors = DEVICE_NORM;

	check_connect_device(&status_sensors);
	if(ctx == NULL){
		set_status_device(DEVICE_CRASH);
		set_status_limit_vertical(DEVICE_LIMIT_VERTICAL);
		set_status_limit_horizontal(DEVICE_LIMIT_HORIZONTAL);
		set_status_pressure(DEVICE_PRESSURE);
		return FALSE;
	}

#if TEST_INTERFACE
	{
		static int rc = 0;
	 	rc ++;
		switch(rc){
			case 1:
				status_sensors = DEVICE_NORM;
				break;
			case 2:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_VERTICAL_TOP;
				break;
			case 3:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_VERTICAL_BOTTOM;
				break;
			case 4:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_LEFT;
				break;
			case 5:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_RIGHT;
				break;
			case 6:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_VERTICAL_TOP;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_RIGHT;
				break;
			case 7:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_VERTICAL_BOTTOM;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_RIGHT;
				break;
			case 8:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_VERTICAL_BOTTOM;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_LEFT;
				break;
			case 9:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_LIMIT_VERTICAL_TOP;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_LEFT;
				break;
			case 10:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_CRASH_VERTICAL;
				break;
			case 11:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_CRASH_HORIZONTAL;
				break;
			case 12:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_CRASH_HORIZONTAL;
				status_sensors += DEVICE_LIMIT_VERTICAL_TOP;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_LEFT;
				break;
			case 13:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_CRASH_HORIZONTAL;
				status_sensors += DEVICE_LIMIT_VERTICAL_BOTTOM;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_LEFT;
				break;
			case 14:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_CRASH_HORIZONTAL;
				status_sensors += DEVICE_LIMIT_VERTICAL_TOP;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_RIGHT;
				break;
			case 15:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_CRASH_HORIZONTAL;
				status_sensors += DEVICE_LIMIT_VERTICAL_BOTTOM;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_RIGHT;
				break;
			case 16:
				status_sensors = DEVICE_NORM;
				status_sensors += DEVICE_PRESSURE;
				status_sensors += DEVICE_LIMIT_HORIZONTAL_RIGHT;
				break;
		}
		if(rc > 17)
			rc = 0;
	}
#endif
	set_status_device(status_sensors);
	set_status_limit_vertical(status_sensors);
	set_status_limit_horizontal(status_sensors);
	set_status_pressure(status_sensors);

	return TRUE;
}

static int time_check_connect_device = DEFAULT_TIMEOU_CHECK_PORT * MILLISECOND;

static int set_status_connect(void)
{
	gtk_label_set_text(GTK_LABEL(lab_status),STR_CONNECT);
	gtk_widget_override_background_color(lab_status,GTK_STATE_FLAG_NORMAL,&color_green);
	gtk_widget_override_color(lab_status,GTK_STATE_FLAG_NORMAL,&color_black);

	gtk_menu_item_set_label(GTK_MENU_ITEM(menite_control_device),STR_OFF_DEVICE);

	g_timeout_add(time_check_connect_device,check_connect_timeout,NULL);

 	return SUCCESS;
}

static int set_status_disconnect(void)
{
	gtk_label_set_text(GTK_LABEL(lab_status),STR_DISCONNECT);
	gtk_widget_override_background_color(lab_status,GTK_STATE_FLAG_NORMAL,&color_red);
	gtk_widget_override_color(lab_status,GTK_STATE_FLAG_NORMAL,&color_white);

	gtk_menu_item_set_label(GTK_MENU_ITEM(menite_control_device),STR_ON_DEVICE);
	return SUCCESS;
}

static char STR_TIMEOUT_CHECK_PORT[] = "timeout_check_port";

static int load_config(void)
{
	int rc;
	GError * err = NULL;
	rc = g_key_file_get_integer(ini_file,STR_GLOBAL_KEY,STR_TIMEOUT_CHECK_PORT,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s",STR_GLOBAL_KEY,STR_TIMEOUT_CHECK_PORT);
		g_error_free(err);
		return FAILURE;
	}
	if( ((rc < MIN_TIMEOUT_CHECK_PORT) || (rc > MAX_TIMEOUT_CHECK_PORT)) ){
		rc = DEFAULT_TIMEOU_CHECK_PORT;
	}

	time_check_connect_device = rc * MILLISECOND;

	return SUCCESS;
}

GtkWidget * create_status_device(void)
{
	GtkWidget * box_horizontal;

	load_config();

	fra_status_connect = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(fra_status_connect),3);
	layout_widget(fra_status_connect,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	box_horizontal = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	layout_widget(box_horizontal,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_box_set_homogeneous(GTK_BOX(box_horizontal),FALSE);

	lab_status = gtk_label_new(STR_DISCONNECT);
	layout_widget(lab_status,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,FALSE,FALSE);
	set_size_font(lab_status,SIZE_FONT_STATUS_DEVICE);
	set_status_disconnect();

	lab_device = gtk_label_new(STR_DEVICE_NORM);
	layout_widget(lab_device,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,FALSE,FALSE);
	set_size_font(lab_device,SIZE_FONT_STATUS_DEVICE);
	set_status_device(DEVICE_CRASH);

	lab_limit_vertical = gtk_label_new(STR_LIMIT_VERTICAL);
	layout_widget(lab_limit_vertical,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,FALSE,FALSE);
	set_size_font(lab_limit_vertical,SIZE_FONT_STATUS_DEVICE);
	set_status_limit_vertical(DEVICE_LIMIT_VERTICAL);

	lab_limit_horizontal = gtk_label_new(STR_LIMIT_HORIZONTAL);
	layout_widget(lab_limit_horizontal,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,FALSE,FALSE);
	set_size_font(lab_limit_horizontal,SIZE_FONT_STATUS_DEVICE);
	set_status_limit_horizontal(DEVICE_LIMIT_HORIZONTAL);

	lab_pressure = gtk_label_new(STR_PRESSURE_NORM);
	layout_widget(lab_pressure,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,FALSE,FALSE);
	set_size_font(lab_pressure,SIZE_FONT_STATUS_DEVICE);
	set_status_pressure(DEVICE_PRESSURE);

	gtk_container_add(GTK_CONTAINER(fra_status_connect),box_horizontal);
	gtk_box_pack_start(GTK_BOX(box_horizontal),lab_status,TRUE,TRUE,2);
	gtk_box_pack_start(GTK_BOX(box_horizontal),lab_device,TRUE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(box_horizontal),lab_limit_vertical,TRUE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(box_horizontal),lab_limit_horizontal,TRUE,FALSE,2);
	gtk_box_pack_start(GTK_BOX(box_horizontal),lab_pressure,TRUE,FALSE,2);

	gtk_widget_show(lab_pressure);
	gtk_widget_show(lab_limit_horizontal);
	gtk_widget_show(lab_limit_vertical);
	gtk_widget_show(lab_device);
	gtk_widget_show(lab_status);
	gtk_widget_show(box_horizontal);
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
	g_message("Установил настройки видео потока.");
}
*/

GtkWidget * create_menu_device(void)
{
	GtkWidget * menite_device;
	GtkWidget * men_device;
	/*GtkWidget * menite_temp;*/

	read_config_device();

	menite_device = gtk_menu_item_new_with_label(STR_DEVICE);

	men_device = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menite_device),men_device);

	menite_control_device = gtk_menu_item_new_with_label(STR_ON_DEVICE);
	g_signal_connect(menite_control_device,"activate",G_CALLBACK(activate_menu_device),NULL);
	gtk_widget_add_accelerator(menite_control_device,"activate",accgro_main
	                          ,GDK_KEY_P,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
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
	                          ,GDK_KEY_S,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_device),menite_temp);
	gtk_widget_show(menite_temp);
*/

	gtk_widget_show(menite_device);

	return menite_device;
}

/*****************************************************************************/
