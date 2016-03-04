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
#define ALLOC_FRAME             TRUE

/*формат видео 16:9 */
/*640*/ /*848*/ /*960*/ /*1280*/ /*1600*/ /*1920*/
/*360*/ /*480*/ /*540*/ /*768 */ /*900 */ /*1080*/
#define DEFAULT_VIDEO_0_WIDTH      960
#define DEFAULT_VIDEO_0_HEIGHT     540

/*формат видео 5:4 */
/*160*/ /*320*/ /*640*/ /*800*/ /*1280*/
/*128*/ /*256*/ /*512*/ /*640*/ /*1024*/
#define DEFAULT_VIDEO_1_WIDTH      640
#define DEFAULT_VIDEO_1_HEIGHT     512

struct _video_stream_s{

	char * name;

	int open;
	GThread * t_video;
	GMutex m_video;

	int number;
	AVFormatContext * format_context;
	AVCodecContext * codec_context;
	struct SwsContext * sws_context;
	AVCodec * codec;

	int exit;

	int x;
	int y;
	int width;
	int height;
	int format_rgb;

	GdkPixbuf * screen;
	int draw;

	/*connect*/
	GtkWidget * check_connect;
	int connect;
	GThread * t_connect;
	GMutex m_connect;

};
typedef struct _video_stream_s video_stream_s;

struct _screen_s{
	GdkPixbuf * image_main;
	GtkWidget * main;

	GtkWidget * win_add;
	GtkMenuItem * menu_add;
	GdkPixbuf * image_add;
	GtkWidget * add;

	video_stream_s * vs0;
	video_stream_s * vs1;
};
typedef struct _screen_s screen_s;

static int deinit_rtsp(video_stream_s * vs);

/*****************************************************************************/
/*  Обработка Видео потока                                                   */
/*****************************************************************************/
/* отдельный поток чтения видео*/
static gpointer read_video_stream(gpointer args)
{
	int rc;
	video_stream_s * vs = (video_stream_s*)args;
	AVPacket packet;
	AVFrame *av_frame = NULL;
	AVFrame *picture_rgb;
#if !ALLOC_FRAME
	uint8_t *buffer;
#endif
	int frame_finish = 0;
	int width;
	int height;

	width = vs->width;
	height = vs->height;

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

#if ALLOC_FRAME
#define ALIGN_BUFFER           32
	av_frame = av_frame_alloc();
	picture_rgb = av_frame_alloc();
	picture_rgb->format = vs->format_rgb;
	picture_rgb->width = width;
	picture_rgb->height = height;
	av_frame_get_buffer(picture_rgb, ALIGN_BUFFER);
#else
	av_frame = avcodec_alloc_frame();
	picture_rgb = avcodec_alloc_frame();
	buffer = g_malloc0(avpicture_get_size(vs->format_rgb,width,height));
	avpicture_fill((AVPicture *)picture_rgb, buffer, vs->format_rgb,width,height);
#endif

	av_read_play(vs->format_context);

	/*параметры преобразования кадра*/
	vs->sws_context = sws_getContext(vs->codec_context->width,vs->codec_context->height,vs->codec_context->pix_fmt
	                                ,width,height,vs->format_rgb
	                                ,SWS_BICUBIC
	                                ,NULL,NULL,NULL);
	for(;;){
		rc = av_read_frame(vs->format_context, &packet);
		if(rc != 0){
			g_critical("Видео потока завершен %s!",vs->name);
			break;
		}
		if(packet.stream_index == vs->number) {
			rc = avcodec_decode_video2(vs->codec_context, av_frame, &frame_finish,&packet);
		if (frame_finish) {
				sws_scale(vs->sws_context,(uint8_t const * const *)av_frame->data, av_frame->linesize,0,vs->codec_context->height
				                  ,picture_rgb->data,picture_rgb->linesize);

				g_mutex_lock(&(vs->m_video));
				if(vs->draw == OK){
					vs->screen = gdk_pixbuf_new_from_data(picture_rgb->data[0],GDK_COLORSPACE_RGB
					                                     ,0,8
					                                     ,width,height
					                                     ,picture_rgb->linesize[0]
					                                     ,NULL,NULL);
					vs->draw = NOT_OK;
				}
				if(vs->exit == OK){
					av_free_packet(&packet);
#if ALLOC_FRAME
					av_frame_free(&av_frame);
					av_frame_free(&picture_rgb);
#else
					g_free(buffer);
					avcodec_free_frame(&av_frame);
					avcodec_free_frame(&picture_rgb);
#endif
					if(vs->draw != OK){
						vs->draw = OK;
					}
					g_mutex_unlock(&(vs->m_video));
					g_thread_exit(0);
				}
				g_mutex_unlock(&(vs->m_video));
			}
		}
		av_free_packet(&packet);
		g_thread_yield();
	}
	vs->open = NOT_OK;
	vs->draw = OK;
	av_free_packet(&packet);
#if ALLOC_FRAME
	av_frame_free(&av_frame);
	av_frame_free(&picture_rgb);
#else
	g_free(buffer);
	avcodec_free_frame(&av_frame);
	avcodec_free_frame(&picture_rgb);
#endif
	deinit_rtsp(vs);
	g_thread_exit(0);
	return NULL;
}

/******  функция запуска по таймеру прорисовки экрана  */
static gboolean write_screen(gpointer ud)
{
	screen_s * s = (screen_s*)ud;
	video_stream_s * vs0 = s->vs0;
	video_stream_s * vs1 = s->vs1;

	if( vs0->open != OK ){
		gdk_pixbuf_fill(s->image_main,0x0);
	}

	if(vs0->draw != OK){
		g_mutex_lock(&(vs0->m_video));
		vs0->draw = OK;
		gdk_pixbuf_copy_area(vs0->screen,0,0,vs0->width,vs0->height,s->image_main,vs0->x,vs0->y);
		g_object_unref(vs0->screen);
		g_mutex_unlock(&(vs0->m_video));
	}
	gtk_image_set_from_pixbuf(GTK_IMAGE(s->main),s->image_main);

	if( s->win_add != NULL){
		if( vs1->open != OK ){
			gdk_pixbuf_fill(s->image_add,0x0);
		}

		if(vs1->draw != OK){
			g_mutex_lock(&(vs1->m_video));
			vs1->draw = OK;
			gdk_pixbuf_copy_area(vs1->screen,0,0,vs1->width,vs1->height,s->image_add,vs1->x,vs1->y);
			g_object_unref(vs1->screen);
			g_mutex_unlock(&(vs1->m_video));
		}
		gtk_image_set_from_pixbuf(GTK_IMAGE(s->add),s->image_add);
	}

	return TRUE;
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
	g_message("Запустил кодек в потоке %s.",vs->name);
	g_message("Инизиализировал видео поток %s размер %dх%d.",vs->name,vs->codec_context->width,vs->codec_context->height);
	return rc;
}

static int deinit_rtsp(video_stream_s * vs)
{
	avcodec_close(vs->codec_context);
	vs->codec_context = NULL;
	avformat_close_input(&(vs->format_context));
	vs->format_context = NULL;
	avformat_network_deinit();
	return SUCCESS;
}

/*****************************************************************************/
/* проверка подсоединения                                                    */
/*****************************************************************************/

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
		g_usleep(3 * G_USEC_PER_SEC);
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
		g_critical("Доступ по адресу %s : %s!",temp_string->str,err->message);
		g_error_free(err);
		return FAILURE;
	}

	g_object_unref(stream);
	return SUCCESS;
}

enum
{
	CONNECT_RTSP_SUCCESS = 1,
	CONNECT_RTSP_FAILURE,
	CONNECT_RTSP_WAIT
};

#define MAX_FPS          100
#define MIN_FPS          1
#define DEFAULT_FPS      25
int FPS = DEFAULT_FPS;
int timeot_fps = MILLISECOND/DEFAULT_FPS; /*40 милесекунд == 25 кадров/с */

static void clicked_button_stop_connect(GtkButton * b,gpointer ud)
{
	video_stream_s * vs = (video_stream_s*)ud;
	GtkWidget * w = vs->check_connect;
	if(w != NULL){
		gtk_widget_destroy(w);
		vs->check_connect = NULL;
	}
	vs->connect = CONNECT_RTSP_FAILURE;
}

/*отдельный поток подключения*/
static gpointer connect_rtsp(gpointer args)
{
	int rc;
	video_stream_s * vs = (video_stream_s*)args;

	/*проверка доступа к камере */
	rc = check_access(vs->name);
	if(rc == FAILURE){
		rc = CONNECT_RTSP_FAILURE;
	}
	else{
		/* подключаем rtsp*/
		rc = init_rtsp(vs);
		if(rc == SUCCESS){
			vs->open = OK;
		}
		rc = CONNECT_RTSP_SUCCESS;
	}

	g_mutex_lock(&(vs->m_connect));
	vs->connect = rc;
	g_mutex_unlock(&(vs->m_connect));

	return NULL;
}

static char STR_NOT_NAME_STREAM[] = "Нет имени потока в файле конфигурации!";
static char STR_NOT_ACCESS_STREAM[] = "Нет доступа к видео потоку : %s";

static int check_connect_rtsp(gpointer ud)
{
	video_stream_s * vs = (video_stream_s*)ud;
	GtkWidget * w = vs->check_connect;
	int connect;

	g_mutex_lock(&(vs->m_connect));
	connect = vs->connect;
	g_mutex_unlock(&(vs->m_connect));

	if(connect == CONNECT_RTSP_SUCCESS){
		g_message("Доступ к %s есть!",vs->name);
		if(w != NULL){
			gtk_widget_destroy(w);
			vs->check_connect = NULL;
			g_mutex_clear(&(vs->m_connect));
		}
		if(vs->open == OK){
			vs->draw = OK;
			vs->exit = NOT_OK;
			g_mutex_init(&(vs->m_video));
			vs->t_video = g_thread_new("video",read_video_stream,vs);
			g_message("Камера %s включена.",vs->name);
		}
		return FALSE;
	}

	if(connect == CONNECT_RTSP_FAILURE){
		g_message("Нет доступа к %s!",vs->name);
		/*сообщение об ошибке*/
		if(w != NULL){
			gtk_widget_destroy(w);
			vs->check_connect = NULL;
			g_mutex_clear(&(vs->m_connect));
		}
		{
		GtkWidget * error;
		if(vs->name == NULL){
			error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
			                              ,GTK_BUTTONS_CLOSE,"%s",STR_NOT_NAME_STREAM);
		}
		else{
			error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
			                              ,GTK_BUTTONS_CLOSE,STR_NOT_ACCESS_STREAM,vs->name);
		}
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		}
		return FALSE;
	}
	return TRUE;
}

static GtkWidget * create_window_connect(video_stream_s * vs)
{
	GtkWidget * dialog;
	GtkWidget * box;
	GtkWidget * label;
	GtkWidget * spinner;
	GtkWidget * button;

	dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dialog),STR_NAME_PROGRAMM);
	gtk_window_set_resizable(GTK_WINDOW(dialog),FALSE);
	gtk_window_set_position (GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(dialog),5);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
	layout_widget(box,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_box_set_homogeneous(GTK_BOX(box),FALSE);

	g_string_printf(temp_string,"Подключение к видеокамере");
	label = gtk_label_new(temp_string->str);
	set_size_font(label,SIZE_FONT_MEDIUM);
	layout_widget(label,GTK_ALIGN_CENTER,GTK_ALIGN_CENTER,FALSE,FALSE);

	spinner = gtk_spinner_new();
	layout_widget(spinner,GTK_ALIGN_CENTER,GTK_ALIGN_CENTER,FALSE,FALSE);
	gtk_widget_set_size_request(spinner,20,20);
	gtk_spinner_start(GTK_SPINNER(spinner));

	button = gtk_button_new_with_label("Завершить подключение");
	g_signal_connect(button,"clicked",G_CALLBACK(clicked_button_stop_connect),vs);
	layout_widget(button,GTK_ALIGN_CENTER,GTK_ALIGN_CENTER,FALSE,FALSE);

	gtk_container_add(GTK_CONTAINER(dialog),box);
	gtk_box_pack_start(GTK_BOX(box),label,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(box),spinner,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(box),button,FALSE,TRUE,0);

	gtk_widget_show(dialog);
	gtk_widget_show(box);
	gtk_widget_show(label);
	gtk_widget_show(spinner);
	gtk_widget_show(button);

	return dialog;
}

#define DEFAULT_TIMEOUT_CHECK_CONNECT_RTSP    500    /*0,5 секунды */
static unsigned long int timeout_check_connect_rtsp = DEFAULT_TIMEOUT_CHECK_CONNECT_RTSP;
static int init_video_stream(video_stream_s * vs)
{
	if(vs->check_connect != NULL){
		g_warning("Подключение уже идет!");
		return FAILURE;
	}

	vs->check_connect = create_window_connect(vs);

	vs->connect = CONNECT_RTSP_WAIT;

	/* запустить функцию  проверки соединения и настройки видео потока*/
	g_mutex_init(&(vs->m_connect));
	vs->t_connect = g_thread_new("connect_rtsp",connect_rtsp,vs);

	/*запустить функцию опроса */
	g_timeout_add(timeout_check_connect_rtsp,check_connect_rtsp,vs);

	return SUCCESS;
}

static int deinit_video_stream(video_stream_s * vs)
{
	if(vs->open == OK){
		vs->exit = OK;
		g_thread_join(vs->t_video);
		g_mutex_clear(&(vs->m_video));
		vs->draw = OK;
		vs->open = NOT_OK;
		deinit_rtsp(vs);
		g_message("Камера %s выключена.",vs->name);
	}
	return SUCCESS;
}

/*****************************************************************************/
/* отображение экрана                                                        */
/*****************************************************************************/

static char STR_VIDEO[] = "Камеры";
static char STR_ON_VIDEO_0[]  = "Включить Основную Камеру";
static char STR_OFF_VIDEO_0[] = "Выключить Основную Камеру";
static char STR_ON_VIDEO_1[]  = "Включить Дополнительную Камеру";
static char STR_OFF_VIDEO_1[] = "Выключить Дополнительную Камеру";

static void clicked_button_window_hide(GtkButton * b,gpointer ud)
{
	GtkWidget * w = (GtkWidget*)ud;
	gtk_widget_destroy(w);
}

static void destroy_window_video_add(GtkWidget * w,gpointer ud)
{
	screen_s * s = (screen_s*)ud;
	GtkMenuItem * mi = s->menu_add;
	video_stream_s * vs_1 = s->vs1;
	GdkPixbuf * buf = s->image_add;

	gtk_menu_item_set_label(mi,STR_ON_VIDEO_1);
	deinit_video_stream(vs_1);
	s->win_add = NULL;
	g_object_unref(buf);
}

static GtkWidget * create_window_video_add(screen_s * s)
{
	GtkWidget * win_add;
	GtkWidget * frame;
	GtkWidget * box;
	GtkWidget * image;
	GdkPixbuf * buf;
	GtkWidget * button;

	win_add = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win_add),STR_NAME_PROGRAMM);
	gtk_window_set_resizable(GTK_WINDOW(win_add),FALSE);
	gtk_window_set_position (GTK_WINDOW(win_add),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(win_add),5);
	g_signal_connect(win_add,"destroy",G_CALLBACK(destroy_window_video_add),s);

	frame = gtk_frame_new("Дополнительная камера");
	layout_widget(frame,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
	layout_widget(box,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_box_set_homogeneous(GTK_BOX(box),FALSE);

	image = gtk_image_new();
	gtk_widget_set_size_request(image,DEFAULT_VIDEO_1_WIDTH,DEFAULT_VIDEO_1_HEIGHT);
	buf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,0,8,DEFAULT_VIDEO_1_WIDTH,DEFAULT_VIDEO_1_HEIGHT);
	s->add = image;
	s->image_add = buf;

	button = gtk_button_new_with_label("Выключить камеру");
	g_signal_connect(button,"clicked",G_CALLBACK(clicked_button_window_hide),win_add);
	layout_widget(button,GTK_ALIGN_CENTER,GTK_ALIGN_CENTER,FALSE,FALSE);

	gtk_container_add(GTK_CONTAINER(win_add),frame);
	gtk_container_add(GTK_CONTAINER(frame),box);
	gtk_box_pack_start(GTK_BOX(box),image,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(box),button,FALSE,TRUE,0);

	gtk_widget_show(win_add);
	gtk_widget_show(frame);
	gtk_widget_show(box);
	gtk_widget_show(image);
	gtk_widget_show(button);

	return win_add;
}

static void activate_menu_video_0(GtkMenuItem * mi,gpointer ud)
{
	screen_s * s = (screen_s*)ud;
	video_stream_s * vs_0 = s->vs0;
	if(vs_0->open != OK){
		gtk_menu_item_set_label(mi,STR_OFF_VIDEO_0);
		init_video_stream(vs_0);
	}
	else{
		gtk_menu_item_set_label(mi,STR_ON_VIDEO_0);
		deinit_video_stream(vs_0);
	}
}

static void activate_menu_video_1(GtkMenuItem * mi,gpointer ud)
{
	screen_s * s = (screen_s*)ud;
	GtkWidget * w = s->win_add;
	video_stream_s * vs_1 = s->vs1;

	if(vs_1->open != OK){
		gtk_menu_item_set_label(mi,STR_OFF_VIDEO_1);
		s->win_add = create_window_video_add(s);
		init_video_stream(vs_1);
	}
	else{
		if(w != NULL){
			gtk_widget_destroy(w);
		}
	}
}

#if 0
static void activate_menu_setting(GtkMenuItem * mi,gpointer ud)
{
	g_message("Установил настройки видео потока.");
}
#endif

static void realize_main_screen(GtkWidget *widget, gpointer data)
{
	screen_s * s = (screen_s*)data;
	video_stream_s * vs_0 = s->vs0;
	video_stream_s * vs_1 = s->vs1;

	vs_0->open = NOT_OK;
	vs_0->draw = OK;
	vs_0->exit = NOT_OK;
	vs_0->x = 0;
	vs_0->y = 0;
	vs_0->width = DEFAULT_VIDEO_0_WIDTH;
	vs_0->height = DEFAULT_VIDEO_0_HEIGHT;
	vs_0->format_rgb = PIX_FMT_RGB24;
	vs_0->check_connect = NULL;
	vs_0->connect = CONNECT_RTSP_FAILURE;

	vs_1->open = NOT_OK;
	vs_1->draw = OK;
	vs_1->exit = NOT_OK;
	vs_1->x = 0;
	vs_1->y = 0;
	vs_1->width = DEFAULT_VIDEO_1_WIDTH;
	vs_1->height = DEFAULT_VIDEO_1_HEIGHT;
	vs_1->format_rgb = PIX_FMT_RGB24;
	vs_1->check_connect = NULL;
	vs_1->connect = CONNECT_RTSP_FAILURE;

	g_timeout_add(timeot_fps,write_screen,s);
}

static void unrealize_main_screen(GtkWidget *widget, gpointer data)
{
}

static char STR_STREAM_0_KEY[] = "stream_0";
static char STR_STREAM_1_KEY[] = "stream_1";
static char STR_FPS_KEY[] = "fps";

static int load_config(screen_s * screen)
{
	GError * err;
	video_stream_s * vs_0	= screen->vs0;
	video_stream_s * vs_1	= screen->vs1;

	err = NULL;
	vs_0->name = g_key_file_get_string (ini_file,STR_VIDEO_KEY,STR_STREAM_0_KEY,&err);
	if(vs_0->name == NULL){
		g_critical("В секции %s нет ключа %s!",STR_VIDEO_KEY,STR_STREAM_0_KEY);
		g_error_free(err);
	}
	else{
		g_message("Основная Камера : %s.",vs_0->name);
	}
	err = NULL;
	vs_1->name = g_key_file_get_string (ini_file,STR_VIDEO_KEY,STR_STREAM_1_KEY,&err);
	if(vs_1->name == NULL){
		g_critical("В секции %s нет ключа %s!",STR_VIDEO_KEY,STR_STREAM_1_KEY);
		g_error_free(err);
	}
	else{
		g_message("Дополнительная Камера : %s.",vs_1->name);
	}

	err = NULL;
	FPS = g_key_file_get_integer(ini_file,STR_VIDEO_KEY,STR_FPS_KEY,&err);
	if(FPS == 0){
		g_critical("В секции %s нет ключа %s!",STR_VIDEO_KEY,STR_FPS_KEY);
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

static video_stream_s video_stream_0 = {0};
static video_stream_s video_stream_1 = {0};
static screen_s screen = {0};

GtkWidget * create_menu_video(void)
{
	GtkWidget * menite_video;
	GtkWidget * men_video;
	GtkWidget * menite_temp;

	menite_video = gtk_menu_item_new_with_label(STR_VIDEO);

	men_video = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menite_video),men_video);

	menite_temp = gtk_menu_item_new_with_label(STR_ON_VIDEO_0);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_video_0),&screen);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_0,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_ON_VIDEO_1);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_video_1),&screen);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_1,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);
	screen.menu_add = GTK_MENU_ITEM(menite_temp);
	/*TODO добавить настройки*/
#if 0
	menite_temp = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_SETTING_VIDEO);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_setting),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'S',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_video),menite_temp);
	gtk_widget_show(menite_temp);
#endif

	gtk_widget_show(menite_video);
	return menite_video;
}

int deinit_video(void)
{
	deinit_video_stream(&video_stream_0);
	deinit_video_stream(&video_stream_1);
	return SUCCESS;
}

GtkWidget * create_video_stream(void)
{
	screen.vs0 = &video_stream_0;
	screen.vs1 = &video_stream_1;

	load_config(&screen);

	screen.main = gtk_image_new();
	gtk_widget_set_size_request(screen.main,DEFAULT_VIDEO_0_WIDTH,DEFAULT_VIDEO_0_HEIGHT);
	screen.image_main = gdk_pixbuf_new(GDK_COLORSPACE_RGB,0,8,DEFAULT_VIDEO_0_WIDTH,DEFAULT_VIDEO_0_HEIGHT);

	g_signal_connect(screen.main,"realize",G_CALLBACK(realize_main_screen),&screen);
	g_signal_connect(screen.main,"unrealize",G_CALLBACK(unrealize_main_screen),&screen);

	screen.win_add = NULL;

	gtk_widget_show(screen.main);

	return screen.main;
}

/*****************************************************************************/
