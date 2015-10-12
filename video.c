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
#define TEST_VIDEO              TRUE

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


static char * name_stream_main = NULL;

static int open_stream = NOT_OK;
static GThread *tid = NULL;
static GMutex mutex;

static int number_video_stream;
static AVFormatContext *av_format_context = NULL;
static AVCodecContext *ac_codec_context = NULL;
static struct SwsContext *sws_ctx = NULL;
static AVCodec *av_codec = NULL;

static int exit_video_stream = NOT_OK;

static GtkWidget * video_stream = NULL;
static GdkPixbuf * image = NULL;
static GdkPixbuf * image_default = NULL;

static int draw_image = OK;

static int deinit_rtsp(void);

static gpointer play_background(gpointer args)
{
	int rc;
	AVPacket packet;
	AVFrame *av_frame = NULL;
	AVFrame *picture_rgb;
	uint8_t *buffer;
	int frameFinished = 0;
	int width = ac_codec_context->width;
	int height = ac_codec_context->height;

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	av_frame = avcodec_alloc_frame();
  picture_rgb = avcodec_alloc_frame();
	buffer = malloc (avpicture_get_size(PIX_FMT_RGB24,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT));
	avpicture_fill((AVPicture *)picture_rgb, buffer, PIX_FMT_RGB24,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);

	av_read_play(av_format_context);

	/*параметры преобразования кадра*/
	sws_ctx = sws_getContext(width,height,ac_codec_context->pix_fmt
	                        ,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT
	                        ,PIX_FMT_RGB24,SWS_BICUBIC
	                        ,NULL,NULL,NULL);
	for(;;){
		rc = av_read_frame(av_format_context, &packet);
		if(rc != 0){
			g_critical("Видео потока завершен!");
			break;
		}
		if(packet.stream_index == number_video_stream) {
			rc = avcodec_decode_video2(ac_codec_context, av_frame, &frameFinished,&packet);
	    if (frameFinished) {
				sws_scale(sws_ctx,(uint8_t const * const *) av_frame->data, av_frame->linesize,0,height
				                  ,picture_rgb->data,picture_rgb->linesize);

				g_mutex_lock(&mutex);
				if(draw_image == OK){
					image = gdk_pixbuf_new_from_data(picture_rgb->data[0],GDK_COLORSPACE_RGB
				                                  ,0,8
																					,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT
																					,picture_rgb->linesize[0]
																					,NULL,NULL);/*,pixmap_destroy_notify,NULL);*/
					draw_image = NOT_OK;
				}
				if(exit_video_stream == OK){
					av_free_packet(&packet);
					if(draw_image != OK){
						draw_image = OK;
						g_object_unref(image);
					}
					g_mutex_unlock(&mutex);
					g_thread_exit(0);
				}
				g_mutex_unlock(&mutex);
			}
		}
		av_free_packet(&packet);
		g_thread_yield();
	}
	open_stream = NOT_OK;
	av_free_packet(&packet);
	deinit_rtsp();
	g_thread_exit(0);
	return NULL;
}

static gboolean play_image(gpointer ud)
{
	if(draw_image != OK){
		g_mutex_lock(&mutex);
		draw_image = OK;
		gtk_image_set_from_pixbuf((GtkImage*) video_stream,image);
		g_object_unref(image);
		g_mutex_unlock(&mutex);
	}
	if(open_stream == OK){
		return TRUE;
	}
	gtk_image_set_from_pixbuf(GTK_IMAGE(video_stream),image_default);
	return 	FALSE;
}

static int init_rtsp(char * name_stream)
{
	int i;
	int rc;
	AVDictionary *optionsDict = NULL;

	if(open_stream == OK){
		g_critical("Видео поток уже открыт!");
		return FAILURE;
	}

  av_register_all();
	avformat_network_init();

	rc = avformat_open_input(&av_format_context,name_stream , NULL, NULL);
	if(rc != 0) {
		g_message("Не смог открыть видео поток");
		return FAILURE;
	}
	g_message("Открыл поток с камеры!");

	rc = avformat_find_stream_info(av_format_context, NULL);
	if(rc < 0){
		g_message("Не нашел поток");
		return FAILURE;
	}
	g_message("Нашел поток");

	/*информация в стандартный поток о видео потоке*/
	av_dump_format(av_format_context, 0, name_stream, 0);

	number_video_stream=-1;
	for(i = 0;i < av_format_context->nb_streams;i++){
		if(av_format_context->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
	    number_video_stream = i;
	    break;
		}
	}
	if(number_video_stream == -1){
		g_message("Не нашел видео поток");
		return FAILURE; // Didn't find a video stream
	}
	g_message("Нашел видео поток");

	ac_codec_context = av_format_context->streams[number_video_stream]->codec;
	av_codec = avcodec_find_decoder(ac_codec_context->codec_id);
	if(av_codec==NULL) {
		g_message("Кодек не поддерживается : %s",av_codec->long_name);
		return FAILURE; // Codec not found
	}
	g_message("Кодек поддерживается : %s",av_codec->long_name);

	rc = avcodec_open2(ac_codec_context, av_codec, &optionsDict);
	if(rc < 0){
		g_message("Не смог открыть кодек");
		return FAILURE;
	}
	g_message("Запустил кодек");
	g_message("Инизиализировал видео поток %dх%d",ac_codec_context->width,ac_codec_context->height);
	return rc;
}

static int deinit_rtsp(void)
{
	/*TODO проверка на освободение памяти буферами кадра*/
	avcodec_close(ac_codec_context);
	ac_codec_context = NULL;
	avformat_close_input(&av_format_context);
	av_format_context = NULL;
	avformat_network_deinit();
	return SUCCESS;
}

#define MAX_FPS          100
#define MIN_FPS          1
#define DEFAULT_FPS      25
int FPS = DEFAULT_FPS;
int timeot_fps = MILLISECOND/DEFAULT_FPS; /*40 милесекунд == 25 кадров/с */

static int init_video_stream(void)
{
	int rc;

	rc = check_name_stream(name_stream_main);
	if(rc == FAILURE){
		return rc;
	}

	rc = init_rtsp(name_stream_main);
	if(rc == SUCCESS){
		open_stream = OK;
	}
	if(open_stream == OK){
		draw_image = OK;
		exit_video_stream = NOT_OK;
		g_mutex_init(&mutex);
		tid = g_thread_new("video",play_background,NULL);
		g_timeout_add(timeot_fps,play_image,NULL);
	 	g_message("Видео запущено");
 	}
	return SUCCESS;
}

int deinit_video_stream(void)
{
	if(open_stream == OK){
		exit_video_stream = OK;
		g_thread_join(tid);
		g_mutex_clear(&mutex);
		deinit_rtsp();
		open_stream = NOT_OK;
		g_message("Видео поток закрыт");
	}
	return SUCCESS;
}

/*****************************************************************************/
static char STR_VIDEO[] = "Камера";
static char STR_ON_VIDEO[]  = "Включить ";
static char STR_OFF_VIDEO[] = "Выключить";
/*static char STR_SETTING_VIDEO[] = "Настройка";*/

static void activate_menu_video(GtkMenuItem * mi,gpointer ud)
{
	if(open_stream != OK){
		gtk_menu_item_set_label(mi,STR_OFF_VIDEO);
		init_video_stream();
	}
	else{
		gtk_menu_item_set_label(mi,STR_ON_VIDEO);
		deinit_video_stream();
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
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_video),NULL);
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
}

static void image_unrealize(GtkWidget *widget, gpointer data)
{
}

static char STR_STREAM_KEY[] = "stream";
static char STR_FPS_KEY[] = "fps";

static int load_config(void)
{
	GError * err = NULL;

	name_stream_main = g_key_file_get_string (ini_file,STR_VIDEO_KEY,STR_STREAM_KEY,&err);
	if(name_stream_main == NULL){
		g_critical("В секции %s нет ключа %s : %s",STR_VIDEO_KEY,STR_STREAM_KEY,err->message);
		g_error_free(err);
	}
	else{
		g_message("Камера : %s",name_stream_main);
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
	GError * err = NULL;

	load_config();

	video_stream = gtk_image_new();
	gtk_widget_set_size_request(video_stream,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);

	image_default = gdk_pixbuf_new_from_file(STR_NAME_DEFAULT_VIDEO,&err);
	if(err != NULL){
		g_message("%s",err->message);
		g_error_free(err);
	}
	else{
		gtk_image_set_from_pixbuf(GTK_IMAGE(video_stream),image_default);
	}

	g_signal_connect(video_stream,"realize",G_CALLBACK(image_realize),NULL);
	g_signal_connect(video_stream,"unrealize",G_CALLBACK(image_unrealize),NULL);
	gtk_widget_show(video_stream);

	return video_stream;
}
/*****************************************************************************/
