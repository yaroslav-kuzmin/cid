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
char VIDEO_GROUP[] = "video";

char *name_stream;
int read_name_stream(void)
{
	GError * err = NULL;

	name_stream = g_key_file_get_string (ini_file,VIDEO_GROUP,"stream",&err);
	if(name_stream == NULL){
		g_message("В секции %s нет ключа %s : %s",VIDEO_GROUP,"stream",err->message);
		g_error_free(err);
		return FAILURE;
	}
	g_message("Камера : %s",name_stream);


	return SUCCESS;
}

#define DEFAULT_VIDEO_WIDTH      720
#define DEFAULT_VIDEO_HEIGHT     576

static int open_stream = NOT_OK;
static GThread *tid = NULL;
static GMutex mutex;

int videoStream;
AVFormatContext *pFormatCtx = NULL;
AVCodecContext *pCodecCtx = NULL;
struct SwsContext *sws_ctx = NULL;
AVCodec *pCodec = NULL;

int exit_video_stream = NOT_OK;

GtkWidget * video_stream = NULL;
GdkPixbuf * image = NULL;
GdkPixbuf * image_default = NULL;

/*
static void pixmap_destroy_notify(guchar *pixels,gpointer data)
{
	return ;
}
*/
int draw_image = OK;

static gpointer play_background(gpointer args)
{
	int rc;
	AVPacket packet;
	AVFrame *pFrame = NULL;
	AVFrame *picture_RGB;
	uint8_t *buffer;
	int frameFinished;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	pFrame = avcodec_alloc_frame();
  picture_RGB = avcodec_alloc_frame();
	buffer = malloc (avpicture_get_size(PIX_FMT_RGB24,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT));
	avpicture_fill((AVPicture *)picture_RGB, buffer, PIX_FMT_RGB24,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);

	av_read_play(pFormatCtx);

	/*параметры преобразования кадра*/
	sws_ctx = sws_getContext(width,height,pCodecCtx->pix_fmt
	                        ,width,height
	                        ,PIX_FMT_RGB24,SWS_BICUBIC
	                        ,NULL,NULL,NULL);
	for(;;){
		rc = av_read_frame(pFormatCtx, &packet);
		if(rc != 0){
			g_message("Ошибка потока ");
			break;
		}
		/*TODO временая задержка для тестирования */
		usleep(50000);
		if(packet.stream_index == videoStream) {
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,&packet);

	    if (frameFinished) {
				sws_scale(sws_ctx,(uint8_t const * const *) pFrame->data, pFrame->linesize,0,height
				                  ,picture_RGB->data,picture_RGB->linesize);

				g_mutex_lock(&mutex);
				if(draw_image == OK){
					image = gdk_pixbuf_new_from_data(picture_RGB->data[0],GDK_COLORSPACE_RGB
				                                  ,0,8,width
																					,height,picture_RGB->linesize[0]
																					,NULL,NULL);/*,pixmap_destroy_notify,NULL);*/

					draw_image = NOT_OK;
				}
				g_mutex_unlock(&mutex);
			}
		}
		av_free_packet(&packet);
		g_thread_yield();
		if(exit_video_stream == OK){
			g_thread_exit(0);
		}
	}

	gtk_image_set_from_pixbuf((GtkImage*) video_stream,image_default);
	open_stream = NOT_OK;
	g_thread_exit(0);
	return NULL;
}

gboolean play_image(gpointer ud)
{
	g_mutex_lock(&mutex);
	if( draw_image != OK){
		draw_image = OK;
		gtk_image_set_from_pixbuf((GtkImage*) video_stream,image);
		/*TODO проверка на высвобождение памяти*/
		g_object_unref(image);
	}
	g_mutex_unlock(&mutex);
	if(open_stream == OK){
		return TRUE;
	}
	return 	FALSE;
}

int init_rtsp(void)
{
	int i;
	int rc;
	AVDictionary *optionsDict = NULL;
	rc = read_name_stream();
	if(rc == FAILURE){
		return rc;
	}

  av_register_all();
	avformat_network_init();

	rc = avformat_open_input(&pFormatCtx,name_stream , NULL, NULL);
	if(rc != 0) {
		g_message("Не смог открыть видео поток");
		return FAILURE;
	}
	g_message("Открыл поток с камеры!");

	rc = avformat_find_stream_info(pFormatCtx, NULL);
	if(rc < 0){
		g_message("Не нашел поток");
		return FAILURE;
	}
	g_message("Нашел поток");

	av_dump_format(pFormatCtx, 0, name_stream, 0);

	videoStream=-1;
	for(i = 0;i < pFormatCtx->nb_streams;i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
	    videoStream = i;
	    break;
		}
	}
	if(videoStream== -1){
		g_message("Не нашел видео поток");
		return FAILURE; // Didn't find a video stream
	}
	g_message("Нашел видео поток");

	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL) {
		g_message("Кодек не поддерживается : %s",pCodec->long_name);
		return FAILURE; // Codec not found
	}
	g_message("Кодек поддерживается : %s",pCodec->long_name);

	rc = avcodec_open2(pCodecCtx, pCodec, &optionsDict);
	if(rc < 0){
		g_message("Не смог открыть кодек");
		return FAILURE;
	}
	g_message("Запустил кодек");
	g_message("Инизиализировал видео поток %dх%d",pCodecCtx->width,pCodecCtx->height);
	return rc;
}

/*****************************************************************************/
int FPS = 40;/*40 милесекунд == 25 кадров/с */

static void image_realize(GtkWidget *widget, gpointer data)
{
	if(open_stream == OK){
		g_mutex_init(&mutex);
		tid = g_thread_new("video",play_background,NULL);
		g_timeout_add(FPS,play_image,NULL);
		g_message("Видео запущено");
	}
}

static void image_unrealize(GtkWidget *widget, gpointer data)
{
	if(open_stream == OK){
		exit_video_stream = OK;
		g_thread_join(tid);
		g_mutex_clear(&mutex);
		avformat_close_input(&pFormatCtx);
		avformat_network_deinit();
		g_message("Ведео поток закрыт");
		open_stream = NOT_OK;
	}
}

GtkWidget * create_video_stream(void)
{
	int rc;
	GError * err = NULL;

	/*TODO  первичная проверка правильности адресса камеры*/

	rc = init_rtsp();
	if(rc == SUCCESS){
		open_stream = OK;
	}

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
