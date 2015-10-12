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
#include <stdint.h>
#include <gtk/gtk.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "total.h"
#include "cid.h"


/*****************************************************************************/
#define TEST_VIDEO              FALSE

static char STR_RTSP[] = "rtsp://";
#define SIZE_STR_RTSP     7
#define DEFAULT_PORT_VIDEO_STREAM   554

static int check_access(char * name)
{
	GSocketConnection	* stream_connect;
	GSocketClient * stream;
	GError * err = NULL;
	char * str = NULL;

	if(name == NULL){
		return FAILURE;
	}

	str = g_strstr_len(name,-1,STR_RTSP);
	if(str == NULL){
#if TEST_VIDEO
		return SUCCESS;
#else
		return FAILURE;
#endif
	}
	str += SIZE_STR_RTSP;

	g_string_printf(temp_string,"%s",str);

	str = g_strstr_len(temp_string->str,-1,"/");
	if(str == NULL){
		return FAILURE;
	}
	*str = 0;

	stream = g_socket_client_new();

	stream_connect = g_socket_client_connect_to_host(stream,temp_string->str
	                                                ,DEFAULT_PORT_VIDEO_STREAM,NULL,&err);
	if(stream_connect == NULL){
		g_critical("Доступ по адресу %s : %s",temp_string->str,err->message);
		g_error_free(err);
		return FAILURE;
	}

	g_object_unref(stream);
	return SUCCESS;
}

static char STR_NOT_NAME_STREAM[] = "Нет имени потока в файле конфигурации!";
static char STR_NOT_ACCESS_STREAM[] = "Нет доступа к видео потоку : %s";

static int check_name_stream(char * name)
{
	int rc;

	rc = check_access(name);
	if(rc != SUCCESS){
		GtkWidget * error;
		if(name == NULL){
			error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
	                                          ,GTK_BUTTONS_CLOSE,"%s",STR_NOT_NAME_STREAM);
		}
		else{
			error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
	                                          ,GTK_BUTTONS_CLOSE,STR_NOT_ACCESS_STREAM,name);
		}
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return FAILURE;
	}
	return SUCCESS;
}
                                        /*формат видео 16:9 */
#define DEFAULT_VIDEO_WIDTH    960      /*640*/ /*848*/ /*960*/ /*1280*/ /*1600*/ /*1920*/
#define DEFAULT_VIDEO_HEIGHT   540      /*360*/ /*480*/ /*540*/ /*768 */ /*900 */ /*1080*/

struct _video_stream_s{

	char * name;

	int open;
	GThread * tid;
	GMutex mutex;

	int number;
	AVFormatContext * format_context;
	AVCodecContext * codec_context;
	struct SwsContext * sws_context;
	AVCodec * codec;

	int exit;

	GdkPixbuf * screen;
	int draw;
};

typedef struct _video_stream_s video_stream_s;

video_stream_s video_stream_0 = {0};

static GdkPixbuf * image_screen = NULL;
static GtkWidget * main_screen = NULL;

static int deinit_rtsp(video_stream_s * vs);

static gpointer read_video_stream(gpointer args)
{
	int rc;
	video_stream_s * vs = (video_stream_s*)args;
	AVPacket packet;
	AVFrame *av_frame = NULL;
	AVFrame *picture_rgb;
	uint8_t *buffer;
	int frameFinished = 0;
	int width = vs->codec_context->width;
	int height = vs->codec_context->height;
	/*TODO проверка утечки памяти*/
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	av_frame = avcodec_alloc_frame();
  picture_rgb = avcodec_alloc_frame();
	buffer = malloc (avpicture_get_size(PIX_FMT_RGB24,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT));
	avpicture_fill((AVPicture *)picture_rgb, buffer, PIX_FMT_RGB24,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);

	av_read_play(vs->format_context);

	/*параметры преобразования кадра*/
	vs->sws_context = sws_getContext(width,height,vs->codec_context->pix_fmt
	                        ,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT
	                        ,PIX_FMT_RGB24,SWS_BICUBIC
	                        ,NULL,NULL,NULL);
	for(;;){
		rc = av_read_frame(vs->format_context, &packet);
		if(rc != 0){
			g_critical("Видео потока завершен!");
			break;
		}
		if(packet.stream_index == vs->number) {
			rc = avcodec_decode_video2(vs->codec_context, av_frame, &frameFinished,&packet);
	    if (frameFinished) {
				sws_scale(vs->sws_context,(uint8_t const * const *) av_frame->data, av_frame->linesize,0,height
				                  ,picture_rgb->data,picture_rgb->linesize);

				g_mutex_lock(&(vs->mutex));
				if(vs->draw == OK){
					vs->screen = gdk_pixbuf_new_from_data(picture_rgb->data[0],GDK_COLORSPACE_RGB
				                                  ,0,8
																					,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT
																					,picture_rgb->linesize[0]
																					,NULL,NULL);
					vs->draw = NOT_OK;
				}
				if(vs->exit == OK){
					av_free_packet(&packet);
					if(vs->draw != OK){
						vs->draw = OK;
					}
					g_mutex_unlock(&(vs->mutex));
					g_thread_exit(0);
				}
				g_mutex_unlock(&(vs->mutex));
			}
		}
		av_free_packet(&packet);
		g_thread_yield();
	}
	vs->open = NOT_OK;
	av_free_packet(&packet);
	deinit_rtsp(vs);
	g_thread_exit(0);
	return NULL;
}

static gboolean write_screen(gpointer ud)
{
	video_stream_s * vs0 = &video_stream_0;

	if(vs0->open != OK){
		gdk_pixbuf_fill(image_screen,0x0);
	}

	if(vs0->draw != OK){
		g_mutex_lock(&(vs0->mutex));
		vs0->draw = OK;
		gdk_pixbuf_copy_area(vs0->screen,0,0,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT,image_screen,0,0);
		g_object_unref(vs0->screen);
		g_mutex_unlock(&(vs0->mutex));
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(main_screen),image_screen);
	return 	TRUE;
}

static int init_rtsp(video_stream_s * vs)
{
	int i;
	int rc;
	AVDictionary *optionsDict = NULL;

	if(vs->open == OK){
		g_critical("Видео поток уже открыт!");
		return FAILURE;
	}

  av_register_all();
	avformat_network_init();

	rc = avformat_open_input(&(vs->format_context),vs->name , NULL, NULL);
	if(rc != 0) {
		g_message("Не смог открыть видео поток");
		return FAILURE;
	}
	g_message("Открыл поток с камеры!");

	rc = avformat_find_stream_info(vs->format_context, NULL);
	if(rc < 0){
		g_message("Не нашел поток");
		return FAILURE;
	}
	g_message("Нашел поток");

	/*информация в стандартный поток о видео потоке*/
	av_dump_format(vs->format_context,0,vs->name, 0);

	vs->number = -1;
	for(i = 0;i < vs->format_context->nb_streams;i++){
		if(vs->format_context->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
	    vs->number = i;
	    break;
		}
	}
	if(vs->number == -1){
		g_message("Не нашел видео поток");
		return FAILURE; // Didn't find a video stream
	}
	g_message("Нашел видео поток");

	vs->codec_context = vs->format_context->streams[vs->number]->codec;
	vs->codec = avcodec_find_decoder(vs->codec_context->codec_id);
	if(vs->codec == NULL) {
		g_message("Кодек не поддерживается : %s",vs->codec->long_name);
		return FAILURE; // Codec not found
	}
	g_message("Кодек поддерживается : %s",vs->codec->long_name);

	rc = avcodec_open2(vs->codec_context, vs->codec, &optionsDict);
	if(rc < 0){
		g_message("Не смог открыть кодек");
		return FAILURE;
	}
	g_message("Запустил кодек");
	g_message("Инизиализировал видео поток %dх%d",vs->codec_context->width,vs->codec_context->height);
	return rc;
}

static int deinit_rtsp(video_stream_s * vs)
{
	/*TODO проверка на освободение памяти буферами кадра*/
	avcodec_close(vs->codec_context);
	vs->codec_context = NULL;
	avformat_close_input(&(vs->format_context));
	vs->format_context = NULL;
	avformat_network_deinit();
	return SUCCESS;
}

#define MAX_FPS          100
#define MIN_FPS          1
#define DEFAULT_FPS      25
int FPS = DEFAULT_FPS;
int timeot_fps = MILLISECOND/DEFAULT_FPS; /*40 милесекунд == 25 кадров/с */

static int init_video_stream_0(void)
{
	int rc;

	rc = check_name_stream(video_stream_0.name);
	if(rc == FAILURE){
		return rc;
	}

	rc = init_rtsp(&video_stream_0);
	if(rc == SUCCESS){
		video_stream_0.open = OK;
	}
	if(video_stream_0.open == OK){
		video_stream_0.draw = OK;
		video_stream_0.exit = NOT_OK;
		g_mutex_init(&(video_stream_0.mutex));
		video_stream_0.tid = g_thread_new("video",read_video_stream,&video_stream_0);
	 	g_message("Видео запущено");
 	}
	return SUCCESS;
}

int deinit_video_stream_0(void)
{
	if(video_stream_0.open == OK){
		video_stream_0.open = NOT_OK;
		video_stream_0.exit = OK;
		g_thread_join(video_stream_0.tid);
		g_mutex_clear(&(video_stream_0.mutex));
		deinit_rtsp(&video_stream_0);
		video_stream_0.draw = NOT_OK;
		g_message("Видео поток закрыт");
	}
	return SUCCESS;
}

/*****************************************************************************/
static char STR_VIDEO[] = "Камера";
static char STR_ON_VIDEO[]  = "Включить ";
static char STR_OFF_VIDEO[] = "Выключить";
/*static char STR_SETTING_VIDEO[] = "Настройка";*/

static void activate_menu_video_0(GtkMenuItem * mi,gpointer ud)
{
	if(video_stream_0.open != OK){
		gtk_menu_item_set_label(mi,STR_OFF_VIDEO);
		init_video_stream_0();
	}
	else{
		gtk_menu_item_set_label(mi,STR_ON_VIDEO);
		deinit_video_stream_0();
	}
}
/*
static void activate_menu_setting(GtkMenuItem * mi,gpointer ud)
{
	g_message("Установил настройки видео потока");
}
*/
GtkWidget * create_menu_video(void)
{
	GtkWidget * menite_video;
	GtkWidget * men_video;
	GtkWidget * menite_temp;

	menite_video = gtk_menu_item_new_with_label(STR_VIDEO);

	men_video = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menite_video),men_video);

	menite_temp = gtk_menu_item_new_with_label(STR_ON_VIDEO);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_video_0),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'V',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);

	/*TODO добавить настройки*/
/*
	menite_temp = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);
	menite_temp = gtk_menu_item_new_with_label(STR_SETTING_VIDEO);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_setting),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'S',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);
*/

	gtk_widget_show(menite_video);
	return menite_video;
}

static void image_realize(GtkWidget *widget, gpointer data)
{
	video_stream_0.open = NOT_OK;
	video_stream_0.draw = OK;
	video_stream_0.exit = NOT_OK;
	g_timeout_add(timeot_fps,write_screen,NULL);
}

static void image_unrealize(GtkWidget *widget, gpointer data)
{
}

static char STR_STREAM_KEY[] = "stream_0";
static char STR_FPS_KEY[] = "fps";

static int load_config(void)
{
	GError * err = NULL;

	video_stream_0.name = g_key_file_get_string (ini_file,STR_VIDEO_KEY,STR_STREAM_KEY,&err);
	if(video_stream_0.name == NULL){
		g_critical("В секции %s нет ключа %s : %s",STR_VIDEO_KEY,STR_STREAM_KEY,err->message);
		g_error_free(err);
	}
	else{
		g_message("Камера : %s",video_stream_0.name);
	}
	err = NULL;
	FPS = g_key_file_get_integer(ini_file,STR_VIDEO_KEY,STR_FPS_KEY,&err);
	if(FPS == 0){
		g_critical("В секции %s нет ключа %s : %s",STR_VIDEO_KEY,STR_FPS_KEY,err->message);
		g_error_free(err);
	}
	else{
		if( (FPS < MIN_FPS) || (FPS > MAX_FPS) ){
			FPS = DEFAULT_FPS;
		}
		timeot_fps = MILLISECOND / FPS;
	}

	return SUCCESS;
}

GtkWidget * create_video_stream(void)
{
	load_config();

	main_screen = gtk_image_new();
	gtk_widget_set_size_request(main_screen,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);
	g_signal_connect(main_screen,"realize",G_CALLBACK(image_realize),NULL);
	g_signal_connect(main_screen,"unrealize",G_CALLBACK(image_unrealize),NULL);

	image_screen = gdk_pixbuf_new(GDK_COLORSPACE_RGB,0,8,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);

	gtk_widget_show(main_screen);

	return main_screen;
}
/*****************************************************************************/
