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
#define DEFAULT_VIDEO_0_WIDTH    960      /*640*/ /*848*/ /*960*/ /*1280*/ /*1600*/ /*1920*/
#define DEFAULT_VIDEO_0_HEIGHT   540      /*360*/ /*480*/ /*540*/ /*768 */ /*900 */ /*1080*/
                                        /*формат видео 5:4 */
#define DEFAULT_VIDEO_1_WIDTH    160      /*160*/ /*320*/ /*640*/ /*800*/ /*1280*/
#define DEFAULT_VIDEO_1_HEIGHT   128      /*128*/ /*256*/ /*512*/ /*640*/ /*1024*/


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

	int width;
	int height;
	int format_rgb;
	GdkPixbuf * screen;
	int draw;
};

typedef struct _video_stream_s video_stream_s;

static video_stream_s video_stream_0 = {0};
static video_stream_s video_stream_1 = {0};

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

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	av_frame = avcodec_alloc_frame();
  picture_rgb = avcodec_alloc_frame();

	buffer = g_malloc0(avpicture_get_size(vs->format_rgb,vs->width,vs->height));
	avpicture_fill((AVPicture *)picture_rgb, buffer, vs->format_rgb,vs->width,vs->height);

	av_read_play(vs->format_context);

	/*параметры преобразования кадра*/
	vs->sws_context = sws_getContext(vs->codec_context->width,vs->codec_context->height,vs->codec_context->pix_fmt
	                                ,vs->width,vs->height,vs->format_rgb
													        ,SWS_BICUBIC
	                                ,NULL,NULL,NULL);
	for(;;){
		rc = av_read_frame(vs->format_context, &packet);
		if(rc != 0){
			g_critical("Видео потока завершен %s!",vs->name);
			break;
		}
		if(packet.stream_index == vs->number) {
			rc = avcodec_decode_video2(vs->codec_context, av_frame, &frameFinished,&packet);
	    if (frameFinished) {
				sws_scale(vs->sws_context,(uint8_t const * const *)av_frame->data, av_frame->linesize,0,vs->codec_context->height
				                  ,picture_rgb->data,picture_rgb->linesize);

				g_mutex_lock(&(vs->mutex));
				if(vs->draw == OK){
					vs->screen = gdk_pixbuf_new_from_data(picture_rgb->data[0],GDK_COLORSPACE_RGB
				                                  ,0,8
																					,vs->width,vs->height
																					,picture_rgb->linesize[0]
																					,NULL,NULL);
					vs->draw = NOT_OK;
				}
				if(vs->exit == OK){
					av_free_packet(&packet);
					g_free(buffer);
					avcodec_free_frame(&av_frame);
					avcodec_free_frame(&picture_rgb);

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
	vs->draw = OK;
	av_free_packet(&packet);
	g_free(buffer);
	avcodec_free_frame(&av_frame);
	avcodec_free_frame(&picture_rgb);
	deinit_rtsp(vs);
	g_thread_exit(0);
	return NULL;
}

static gboolean write_screen(gpointer ud)
{
	video_stream_s * vs0 = &video_stream_0;
	video_stream_s * vs1 = &video_stream_1;

	if( (vs0->open != OK) && (vs1->open != OK) ){
		gdk_pixbuf_fill(image_screen,0x0);
	}

	if(vs0->draw != OK){
		g_mutex_lock(&(vs0->mutex));
		vs0->draw = OK;
		gdk_pixbuf_copy_area(vs0->screen,0,0,vs0->width,vs0->height,image_screen,0,0);
		g_object_unref(vs0->screen);
		g_mutex_unlock(&(vs0->mutex));
	}
	if(vs1->draw != OK){
		g_mutex_lock(&(vs1->mutex));
		vs1->draw = OK;
		gdk_pixbuf_copy_area(vs1->screen,0,0,vs1->width,vs1->height,image_screen,0,0);
		g_object_unref(vs1->screen);
		g_mutex_unlock(&(vs1->mutex));
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
		g_critical("Видео поток %s уже открыт!",vs->name);
		return FAILURE;
	}

  av_register_all();
	avformat_network_init();

	rc = avformat_open_input(&(vs->format_context),vs->name , NULL, NULL);
	if(rc != 0) {
		g_critical("Не смог открыть поотк %s!",vs->name);
		return FAILURE;
	}
	g_message("Открыл поток %s.",vs->name);

	rc = avformat_find_stream_info(vs->format_context, NULL);
	if(rc < 0){
		g_critical("Не нашел поток %s!",vs->name);
		return FAILURE;
	}
	g_message("Нашел поток %s.",vs->name);

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
		g_critical("Не нашел видео поток %s!",vs->name);
		return FAILURE; // Didn't find a video stream
	}
	g_message("Нашел видео поток %s.",vs->name);

	vs->codec_context = vs->format_context->streams[vs->number]->codec;
	vs->codec = avcodec_find_decoder(vs->codec_context->codec_id);
	if(vs->codec == NULL) {
		g_critical("Кодек не поддерживается %s в видео потоке %s! ",vs->codec->long_name,vs->name);
		return FAILURE; // Codec not found
	}
	g_message("Кодек поддерживается %s в видео потоке %s.",vs->codec->long_name,vs->name);

	rc = avcodec_open2(vs->codec_context, vs->codec, &optionsDict);
	if(rc < 0){
		g_critical("Не смог открыть кодек в потоке %s!",vs->name);
		return FAILURE;
	}
	g_message("Запустил кодек в потоке %s",vs->name);
	g_message("Инизиализировал видео поток %s размер %dх%d",vs->name,vs->codec_context->width,vs->codec_context->height);
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


static int init_video_stream(video_stream_s * vs)
{
	int rc;

	rc = check_name_stream(vs->name);
	if(rc == FAILURE){
		return rc;
	}

	rc = init_rtsp(vs);
	if(rc == SUCCESS){
		vs->open = OK;
	}
	if(vs->open == OK){
		vs->draw = OK;
		vs->exit = NOT_OK;
		g_mutex_init(&(vs->mutex));
		vs->tid = g_thread_new("video",read_video_stream,vs);
	 	g_message("Видео %s запущено",vs->name);
 	}
	return SUCCESS;
}

static int deinit_video_stream(video_stream_s * vs)
{
	if(vs->open == OK){
		vs->exit = OK;
		g_thread_join(vs->tid);
		g_mutex_clear(&(vs->mutex));
		vs->draw = OK;
		vs->open = NOT_OK;
		deinit_rtsp(vs);
		g_message("Видео %s закрыто",vs->name);
	}
	return SUCCESS;
}

int deinit_video(void)
{
	deinit_video_stream(&video_stream_0);
	deinit_video_stream(&video_stream_1);
	return SUCCESS;
}
/*****************************************************************************/
static char STR_VIDEO[] = "Камеры";
static char STR_ON_VIDEO_0[]  = "Включить Камеру 0";
static char STR_OFF_VIDEO_0[] = "Выключить Камеру 0";
static char STR_ON_VIDEO_1[]  = "Включить Камеру 1";
static char STR_OFF_VIDEO_1[] = "Выключить Камеру 1";
/*static char STR_SETTING_VIDEO[] = "Настройка";*/

static void activate_menu_video_0(GtkMenuItem * mi,gpointer ud)
{
	if(video_stream_0.open != OK){
		gtk_menu_item_set_label(mi,STR_OFF_VIDEO_0);
		init_video_stream(&video_stream_0);
	}
	else{
		gtk_menu_item_set_label(mi,STR_ON_VIDEO_0);
		deinit_video_stream(&video_stream_0);
	}
}
static void activate_menu_video_1(GtkMenuItem * mi,gpointer ud)
{
	if(video_stream_1.open != OK){
		gtk_menu_item_set_label(mi,STR_OFF_VIDEO_1);
		init_video_stream(&video_stream_1);
	}
	else{
		gtk_menu_item_set_label(mi,STR_ON_VIDEO_1);
		deinit_video_stream(&video_stream_1);
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

	menite_temp = gtk_menu_item_new_with_label(STR_ON_VIDEO_0);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_video_0),NULL);
/*	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'V',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);*/
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_ON_VIDEO_1);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_video_1),NULL);
/*	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'V',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);*/
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

static void realize_main_screen(GtkWidget *widget, gpointer data)
{
	video_stream_0.open = NOT_OK;
	video_stream_0.draw = OK;
	video_stream_0.exit = NOT_OK;
	video_stream_0.width = DEFAULT_VIDEO_0_WIDTH;
	video_stream_0.height = DEFAULT_VIDEO_0_HEIGHT;
	video_stream_0.format_rgb = PIX_FMT_RGB24;

	video_stream_1.open = NOT_OK;
	video_stream_1.draw = OK;
	video_stream_1.exit = NOT_OK;
	video_stream_1.width = DEFAULT_VIDEO_1_WIDTH;
	video_stream_1.height = DEFAULT_VIDEO_1_HEIGHT;
	video_stream_1.format_rgb = PIX_FMT_RGB24;

	g_timeout_add(timeot_fps,write_screen,NULL);
}

static void unrealize_main_screen(GtkWidget *widget, gpointer data)
{
}

static char STR_STREAM_0_KEY[] = "stream_0";
static char STR_STREAM_1_KEY[] = "stream_1";
static char STR_FPS_KEY[] = "fps";

static int load_config(void)
{
	GError * err;

	err = NULL;
	video_stream_0.name = g_key_file_get_string (ini_file,STR_VIDEO_KEY,STR_STREAM_0_KEY,&err);
	if(video_stream_0.name == NULL){
		g_critical("В секции %s нет ключа %s : %s",STR_VIDEO_KEY,STR_STREAM_0_KEY,err->message);
		g_error_free(err);
	}
	else{
		g_message("Камера 0 : %s",video_stream_0.name);
	}
	err = NULL;
	video_stream_1.name = g_key_file_get_string (ini_file,STR_VIDEO_KEY,STR_STREAM_1_KEY,&err);
	if(video_stream_1.name == NULL){
		g_critical("В секции %s нет ключа %s : %s",STR_VIDEO_KEY,STR_STREAM_1_KEY,err->message);
		g_error_free(err);
	}
	else{
		g_message("Камера 1 : %s",video_stream_1.name);
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
	gtk_widget_set_size_request(main_screen,DEFAULT_VIDEO_0_WIDTH,DEFAULT_VIDEO_0_HEIGHT);
	g_signal_connect(main_screen,"realize",G_CALLBACK(realize_main_screen),NULL);
	g_signal_connect(main_screen,"unrealize",G_CALLBACK(unrealize_main_screen),NULL);

	image_screen = gdk_pixbuf_new(GDK_COLORSPACE_RGB,0,8,DEFAULT_VIDEO_0_WIDTH,DEFAULT_VIDEO_0_HEIGHT);

	gtk_widget_show(main_screen);

	return main_screen;
}
/*****************************************************************************/
