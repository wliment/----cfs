#include <stdio.h>
#include "thread_cfs.h"
#include <string.h>
#include <stdlib.h>

GtkTextBuffer *info_area;
GtkTextTag *tag;
GtkTextTag *blue_tag;
GtkTextView *textview2;

//调度输出相关全局变量
GtkTextBuffer *sched_info_area;
GtkTextView *sched_textview;


//进程初始化信息全局变量
GtkTextBuffer *init_info_area;
GtkTextView  *init_textview;


FILE *sched_info_str_buffer;
FILE *sched_output;

struct status_update_info{
GtkWidget *cfs_nrruning;
GtkWidget *min_vruntime;
};

struct image_update{
GtkWidget *image_window;
GtkWidget *image;
};
//输出进程初始话信息
void put_init_info(struct sched_entity *se)
{

 char str[100];
 sprintf(str,"进程[ %5d ]: 优先级: %4d  运算数: %5d 权重: %8lu\n",se->p_pid,PRIO_TO_NICE(se->static_prio),se->cal_num,se->load.weight);
 printf(str);
 gdk_threads_enter();
 GtkTextIter iter;
 gtk_text_buffer_get_iter_at_offset(init_info_area, &iter, 0);
 gtk_text_buffer_insert_with_tags(init_info_area, &iter,str,strlen(str),blue_tag,NULL);
 gdk_threads_leave();


}
void  put_exit_info(struct sched_entity *se) //输出进程退出信息
{
gdk_threads_enter();
  char str[50];
  GtkAdjustment *hscroll;
  sprintf(str,"进程[ %5d ]已经运行完成",se->p_pid);
  GtkTextIter iii;
  gtk_text_buffer_get_iter_at_offset( info_area, &iii, -1);
  gtk_text_buffer_insert_with_tags(info_area, &iii,str,strlen(str),tag,NULL);

  sprintf(str,"\t进程优先级:%4d\t运算步数: %5d\n",PRIO_TO_NICE(se->static_prio),se->cal_num);
  gtk_text_buffer_get_iter_at_offset( info_area, &iii, -1);
  gtk_text_buffer_insert_with_tags(info_area, &iii,str,strlen(str),blue_tag,NULL);

  GtkTextMark *mark = NULL;
  GtkTextIter iter_start;
  GtkTextIter iter_end;
  gtk_text_buffer_get_bounds(info_area, &iter_start, &iter_end);
  mark = gtk_text_buffer_create_mark(info_area, NULL, &iter_end, FALSE);
 while (gtk_events_pending ())
  gtk_main_iteration ();
  gtk_text_view_scroll_mark_onscreen(textview2, mark);
gdk_threads_leave();
}

void get_sched_info(struct sched_entity *prev,struct sched_entity *curr)//输出调度信息
{
  fprintf(sched_info_str_buffer,"进程[ %5d ]被换入 <--> 进程［ %5d ]被换出\n",curr->p_pid,prev->p_pid);
}

int put_sched_info() 
{


  GtkAdjustment *hscroll;
  GtkTextIter start;
  GtkTextIter end;

  if(cfs->nr_running == 0)
  {
       return 0;
  }

  gdk_threads_enter();
  /*fseek(sched_info_str_buffer, 0L, SEEK_END ); //获取调度信息中的内容*/
  int size =57*1000;
  char *text=(char*)malloc(size);
  fread(text,sizeof(char),size,sched_output);
printf("bbbbbbbbbbbbbbbbbbbbbbbbb %d\n",strlen(text));
  /*gtk_text_buffer_get_start_iter(sched_info_area, &start);//清空textbuffer*/
  /*gtk_text_buffer_get_end_iter(sched_info_area, &end);*/
  /*gtk_text_buffer_delete (sched_info_area,&start,&end);*/

  gtk_text_buffer_get_iter_at_offset(sched_info_area, &end, -1); //插入缓冲区
  gtk_text_buffer_insert_with_tags(sched_info_area, &end,text,size,tag,NULL);
  GtkTextMark *mark = NULL;

  gtk_text_buffer_get_bounds(sched_info_area, &start, &end);
  mark = gtk_text_buffer_create_mark(sched_info_area, NULL, &end, FALSE);
  gtk_text_view_scroll_mark_onscreen(sched_textview, mark);
     
  gdk_threads_leave();
  free(text);
  return 1;
}
void update_info(struct status_update_info *sui)
{


  if(cfs->nr_running == 0)
  {
       return 0;
  }
  gchar *str;
	str = g_strdup_printf ("%d", cfs->nr_running);
  gtk_label_set_text(GTK_LABEL(sui->cfs_nrruning),str);
	str = g_strdup_printf ("%llu", cfs->min_vruntime);
  gtk_label_set_text(GTK_LABEL(sui->min_vruntime),str);
	g_free (str);
  return 1;
}


int update_cfs_tree(GtkImage *image)
{
  if(!if_output_tree)
  {
       return 0;
  }
  gtk_image_set_from_file (image,"./test.svg");
  return 1;
}

//双态按钮回调
void on_togglebutton1_toggled( GtkToggleButton *toggleBtn,gpointer data)
{
  struct image_update *iu = (struct image_update *)data; 
  if(gtk_toggle_button_get_active(toggleBtn)) {
    gtk_button_set_label(GTK_BUTTON(toggleBtn), "暂停");
    if_output_tree = 1;
    g_timeout_add(300, (GSourceFunc) tree_to_image,NULL );
    g_timeout_add(500, (GSourceFunc)update_cfs_tree,iu->image);
    gtk_widget_show_all(iu->image_window);
gtk_window_set_default_size(GTK_WINDOW(iu->image_window), 1000, 800);
gtk_window_set_resizable (GTK_WINDOW(iu->image_window), FALSE);
  } else {
    gtk_button_set_label(GTK_BUTTON(toggleBtn), "开始");
    gtk_widget_hide_all(iu->image_window);
    if_output_tree = 0;
  }

}

void *gtk (int argc, char *argv[]) //图形界面主线程
{
  GtkBuilder *builder;
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *textview;
  GtkTextBuffer *buffer;
  GtkTextTag *ttt;
  GtkTextTagTable *tag_table;

  GtkTextIter iter;

  g_thread_init(NULL);
  gtk_init (&argc, &argv);
  gdk_threads_init();

  builder = gtk_builder_new();

  gtk_builder_add_from_file(builder, "cfs.glade", NULL);
  window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  button = GTK_WIDGET(gtk_builder_get_object(builder,"button3"));
  textview2 = GTK_WIDGET(gtk_builder_get_object(builder,"textview1"));
  info_area =  (GtkTextBuffer *)gtk_builder_get_object(builder,"textbuffer1");

  sched_textview  =  GTK_WIDGET(gtk_builder_get_object(builder,"textview2"));
  sched_info_area =  (GtkTextBuffer *)gtk_builder_get_object(builder,"textbuffer2");


  init_textview = GTK_WIDGET(gtk_builder_get_object(builder,"textview3"));
  init_info_area = (GtkTextBuffer *)gtk_builder_get_object(builder,"textbuffer3");




  ttt = (GtkTextTag *)gtk_builder_get_object(builder,"pexit");
  tag=ttt;
  blue_tag = (GtkTextTag *)gtk_builder_get_object(builder,"blue_tag");
  tag_table = (GtkTextTagTable *)gtk_builder_get_object(builder,"tab");
  gtk_text_tag_table_add(tag_table,ttt);
  gtk_text_tag_table_add(tag_table,blue_tag);
  
GdkColor color;
gtk_widget_realize(window);
gdk_color_parse("black", &color); 
gtk_widget_modify_base(textview2, GTK_STATE_NORMAL , &color);
gtk_widget_modify_base(init_textview, GTK_STATE_NORMAL , &color);
gtk_widget_modify_base(sched_textview, GTK_STATE_NORMAL , &color);
gtk_text_buffer_get_iter_at_offset(sched_info_area, &iter, 0);
GtkTextTag *blue_fg = gtk_text_buffer_create_tag(sched_info_area, "blue_fg", "foreground", "red", NULL);



//更新status
GtkWidget *cfs_nrruning,*min_vruntime;
struct status_update_info sui;
cfs_nrruning=  GTK_WIDGET(gtk_builder_get_object(builder,"nr_runing_val"));
min_vruntime = GTK_WIDGET(gtk_builder_get_object(builder,"min_vruntime_val"));
sui.cfs_nrruning = cfs_nrruning;
sui.min_vruntime = min_vruntime;



struct image_update iu;
GtkToggleButton *toggleBtn;
GtkWindow *image_window;
GtkImage *image;
toggleBtn = GTK_WIDGET(gtk_builder_get_object(builder,"togglebutton1"));
iu.image_window = GTK_WIDGET(gtk_builder_get_object(builder,"rbtree_image"));
iu.image  = GTK_WIDGET(gtk_builder_get_object(builder,"image1"));
g_signal_connect( G_OBJECT(toggleBtn), "toggled", G_CALLBACK(on_togglebutton1_toggled),&iu);


g_signal_connect (window, "destroy", G_CALLBACK (gtk_widget_destroyed), &window);

  /*gtk_text_buffer_insert_with_tags(sched_info_area, &iter,"sdfsdfsdfsdfsdfsdfsdf\n",-1,tag,NULL);*/
 /*g_signal_connect( G_OBJECT(button), "clicked", G_CALLBACK(on_button3_clicked),NULL);*/
/*char *sss="进程[ 23181 ]被换入 <--> 进程［ 23194 ]被换出\n";*/
/*printf("mmmmmmmmmmmmmmmmm:%d\n",strlen(sss));*/
  gtk_builder_connect_signals(builder, NULL);
  sleep(1);
  /*g_timeout_add(100, (GSourceFunc) update_cfs_tree, (gpointer)image);*/
  g_timeout_add(1000, (GSourceFunc) put_sched_info, NULL);
  g_timeout_add(200, (GSourceFunc)update_info,&sui);
  g_object_unref(G_OBJECT(builder));
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
