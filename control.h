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

#ifndef CONTROL_H
#define CONTROL_H

GtkWidget * create_status_device(void);
GtkWidget * create_menu_device(void);

int check_connect_device(uint16_t * status);
int deinit_control_device(void);

int command_wait_mode(void);
int command_config_mode(void);
int command_auto_mode(void);
int command_manual_mode(void);
int command_auto_null(void);
int command_auto_start(void);
int command_auto_pause(void);
int command_auto_stop(void);
int command_manual_null(void);
int command_manual_up_on(void);
int command_manual_up_off(void);
int command_manual_down_on(void);
int command_manual_down_off(void);
int command_manual_left_on(void);
int command_manual_left_off(void);
int command_manual_right_on(void);
int command_manual_right_off(void);
int command_manual_drive_on(void);
int command_manual_drive_off(void);
int command_manual_laser_on(void);
int command_manual_laser_off(void);
int command_uprise_angle(int value);
int command_lowering_angle(int value);
int command_pressure(uint16_t pressure);
int command_speed_vertical(uint16_t speed);
int command_valve(uint16_t value);
int command_horizontal(uint16_t value);
int command_null_mode(void);

int info_angle(uint16_t * val);
int info_pressure(uint16_t * val);
int info_sensors(uint16_t * val);
int info_input(uint16_t * val);
int info_console(void);
int info_console_up(void);
int info_console_down(void);
int info_console_left(void);
int info_console_right(void);
int info_console_pause(void);
int info_console_stop(void);
int info_console_on_valve(void);
int info_console_off_valve(void);


#endif

