###############################################################################
#                                                                             #
# УПО - Управление Промышленым Оборудованием                                  #
#  Авторское Право (C) <2015> <Кузьмин Ярослав>                               #
#                                                                             #
#  Эта программа является свободным программным обеспечением:                 #
#  вы можете распространять и/или изменять это в соответствии с               #
#  условиями в GNU General Public License, опубликованной                     #
#  Фондом свободного программного обеспечения, как версии 3 лицензии,         #
#  или (по вашему выбору) любой более поздней версии.                         #
#                                                                             #
#  Эта программа распространяется в надежде, что она будет полезной,          #
#  но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ; даже без подразумеваемой гарантии              #
#  Или ПРИГОДНОСТИ ДЛЯ КОНКРЕТНЫХ ЦЕЛЕЙ. См GNU General Public License        #
#  для более подробной информации.                                            #
#                                                                             #
#  Вы должны были получить копию GNU General Public License                   #
#  вместе с этой программой. Если нет, см <http://www.gnu.org/licenses/>      #
#                                                                             #
#  Адрес для контактов: kuzmin.yaroslav@gmail.com                             #
#                                                                             #
###############################################################################
#                                                                             #
# cid - control industrial device                                             #
#  Copyright (C) <2015> <Kuzmin Yaroslav>                                     #
#                                                                             #
#  This program is free software: you can redistribute it and/or modify       #
#  it under the terms of the GNU General Public License as published by       #
#  the Free Software Foundation, either version 3 of the License, or          #
#  (at your option) any later version.                                        #
#                                                                             #
#  This program is distributed in the hope that it will be useful,            #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of             #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              #
#  GNU General Public License for more details.                               #
#                                                                             #
#  You should have received a copy of the GNU General Public License          #
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.      #
#                                                                             #
#  Email contact: kuzmin.yaroslav@gmail.com                                   #
#                                                                             #
###############################################################################

OBJ_CATALOG=.obj/
DEPEND_CATALOG=.depend/

MODBUS_CATALOG=libmodbus/
LIB_MODBUS=$(MODBUS_CATALOG)libmodbus.a
LIB_MODBUS_OPTION=-lmodbus

EXEC=cid.exe
SOURCE=$(wildcard *.c)
OBJS=$(patsubst %.c,$(OBJ_CATALOG)%.o,$(SOURCE))
DEPEND=$(patsubst %.c,$(DEPEND_CATALOG)%.d,$(SOURCE))

CXX=gcc
CFLAGS=-O -Wall -I. -I$(MODBUS_CATALOG) `pkg-config --cflags gtk+-3.0`
LDFLAGS=-O -L$(MODBUS_CATALOG) -mwindows
LIB=`pkg-config --libs gtk+-3.0` -lavformat -lavcodec -lswscale -lavutil -lsqlite3 

RESOURCE_CATALOG=resource/
RC=rcedit
ICON=$(RESOURCE_CATALOG)cid.ico

RESOURCE=$(RESOURCE_CATALOG)cid.gresource.xml
SOURCE_RESOURCE=$(RESOURCE_CATALOG)cid.c
HEADRE_RESOURCE=$(RESOURCE_CATALOG)cid.h
OBJ_RESOURCE=$(RESOURCE_CATALOG)cid.o
COMPILE_RESOURCE=glib-compile-resources 

HEADER_VERSION=version.h
GIT_REPOSITOR=.git
GIT_VERSION=./git_version.sh

$(EXEC):$(OBJS) $(OBJ_RESOURCE) $(LIB_MODBUS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIB)
	$(RC) $(EXEC) --set-icon $(ICON)

$(LIB_MODBUS):$(MODBUS_CATALOG)
	make -C $<

$(OBJ_CATALOG)%.o:%.c
	$(CXX) $(CFLAGS) -c $< -o $@

$(DEPEND_CATALOG)%.d:%.c
	$(CXX) -MM -I. -I$(MODBUS_CATALOG) $< | sed -e '1s/^/.obj\//' > $@

include $(DEPEND)

$(OBJ_RESOURCE):$(SOURCE_RESOURCE) $(HEADRE_RESOURCE)
	$(CXX) $(CFLAGS) -c $< -o $@
	
$(SOURCE_RESOURCE):$(RESOURCE)
	$(COMPILE_RESOURCE) --target=$@ --generate-source $<
	
$(HEADRE_RESOURCE):$(RESOURCE)
	$(COMPILE_RESOURCE) --target=$@ --generate-header $<

$(HEADER_VERSION):$(GIT_REPOSITOR)
	$(GIT_VERSION)
	
.PHONY:clean
clean:
	-rm -f $(EXEC) *~ $(OBJ_CATALOG)*.o $(DEPEND_CATALOG)*.d $(OBJ_RESOURCE) $(SOURCE_RESOURCE) $(HEADRE_RESOURCE)

