#include <QCoreApplication>
#include <QDir>
#include <QDate>
#include <QDebug>
#include <qthread.h>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlError>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <iostream>
#include <fstream>
#include <string>

#include <mutex>
#include <condition_variable>
#include <chrono>

GstElement *sink = nullptr;

//#include "GstBaseDvr.h"
//Data structre to capsulate needed modules for writing operation.
typedef struct
{
    int branchIndex;
}GstData;

typedef struct
{
    GstElement *capturePipe;
    GstElement *writerPipe;
    GMainLoop *mainLoop;
}BusMesageEventData;
////***********************************************************
////Declare a function to handle messages from capture pipe.
static gboolean onCapturePipeMessage(
    GstBus *bus,
    GstMessage *message,
    gpointer data);
//***********************************************************
static int16_t onNewSampleFromCapture(GstElement *elt, gpointer data);
//***********************************************************
bool isStopCall = false;
bool isStopperThreadFreed = false;

std::mutex protectionMutex;

std::condition_variable conditionalBlock;

GstPad *srcPadOfFileSink = nullptr;
GstPad *sinkPadOfFileSink = nullptr;
//***********************************************************
GstPadProbeReturn handleSinkPadMsgs(
    GstPad *pad,
    GstPadProbeInfo *info,
    gpointer data)
{

    if (info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM ||
        info->type & GST_PAD_PROBE_TYPE_EVENT_FLUSH)
    {
        GstEvent *event = gst_pad_probe_info_get_event (info);

        switch (GST_EVENT_TYPE (event))
        {
            case GST_EVENT_EOS:
                qDebug() << "got eos event in sink "
                            "element of splitmuxsink\n";
                if (true == isStopCall)
                {
                    qDebug() << "*************try to notify thread 2\n";
                    while (false == isStopperThreadFreed)
                    {
                        conditionalBlock.notify_one();
                    }
                    qDebug() << "**********After freeing thread 2\n";
                    isStopCall = false;
                }
                break;
            case GST_EVENT_FLUSH_START:
                qDebug() << "got flush start event in sink "
                            "element of splitmuxsink\n";
                break;
        }
    }
    return GST_PAD_PROBE_OK;
}
//***********************************************************
GstStateChangeReturn (*mainStateChange)
    (GstElement * element, GstStateChange transition)= nullptr;

GstStateChangeReturn myStateChange
    (GstElement * element, GstStateChange transition)
{
    qDebug() << "**************my state changed is called \n";
    GstStateChangeReturn res = GST_STATE_CHANGE_SUCCESS;

    switch (transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY:
        qDebug() << "*******cange state is GST_STATE_CHANGE_NULL_TO_READY\n";
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        qDebug() << "*******cange state is GST_STATE_CHANGE_READY_TO_PAUSED\n";
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        qDebug() << "*******cange state is GST_STATE_CHANGE_PAUSED_TO_PLAYING\n";
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        qDebug() << "*******cange state is GST_STATE_CHANGE_PLAYING_TO_PAUSED\n";
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        qDebug() << "*******cange state is GST_STATE_CHANGE_PAUSED_TO_READY\n";
        break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        qDebug() << "*******cange state is GST_STATE_CHANGE_READY_TO_NULL\n";
        break;
    case GST_STATE_CHANGE_NULL_TO_NULL:
        qDebug() << "*******cange state is GST_STATE_CHANGE_NULL_TO_NULL\n";
        break;
    case GST_STATE_CHANGE_READY_TO_READY:
        qDebug() << "*******cange state is GST_STATE_CHANGE_READY_TO_READY\n";
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PAUSED:
        qDebug() << "*******cange state is GST_STATE_CHANGE_PAUSED_TO_PAUSED\n";
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PLAYING:
        qDebug() << "*******cange state is GST_STATE_CHANGE_PLAYING_TO_PLAYING\n";
        break;
    }

    if (nullptr != mainStateChange)
    {
        res = mainStateChange(element, transition);
    }
    return res;
}
//***********************************************************
//                  test AutoResetEvent module
//***********************************************************
#include <QElapsedTimer>
#include <QSqlRecord>
//***********************************************************
int main(int argc, char *argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    GMainLoop *mainLoop;

    GstBus *gstBus = nullptr;
    //Declare some needed variables to create gstreamer capture pipe;
    GstElement *videoCapturePipe= nullptr;
    guint videoCaptureBusWatchId;
    GstElement *captureOutGate = nullptr;
    GstData programData;
    GstData programData2;

    mainLoop = g_main_loop_new(nullptr, FALSE);

    //Read gstreamer pipe from file to read video from capture
    std::string gstCapturePipe;
    std::ifstream gstPipeFile("/home/nvidia/save_gstsample_data/gst.txt");

    if (true == gstPipeFile.is_open())
    {
        gstPipeFile.seekg(0, std::ios::end);
        gstCapturePipe.reserve(gstPipeFile.tellg());
        gstPipeFile.seekg(0, std::ios::beg);

        gstCapturePipe.assign(
            (std::istreambuf_iterator<char>(gstPipeFile)),
            std::istreambuf_iterator<char>());

        std::cout << gstCapturePipe << std::endl;
        gstPipeFile.close();
    }
    else
    {
        //Gst pipe to capture video from onboard camera and
        //stream it to appsink
        gstCapturePipe =
            "nvarguscamerasrc ! "
            "video/x-raw(memory:NVMM), "
            "width=(int)1920, height=(int)1080, "
            "format=(string)NV12, framerate=(fraction)30/1 ! "
            "nvvidconv ! "
            "video/x-raw, "
            "width=(int)1920, height=(int)1080, "
            "format=(string)BGRx, framerate=(fraction)30/1 ! "
            "queue ! appsink name=videoCaptureSink";
    }
    //Genarate gstreamer capture pipe
    videoCapturePipe = gst_parse_launch(gstCapturePipe.c_str(), nullptr);
    if (nullptr == videoCapturePipe)
    {
        g_print("Bad source\n");
        g_main_loop_unref(mainLoop);
        return -1;
    }
    BusMesageEventData busMessageEventData;
    busMessageEventData.capturePipe = videoCapturePipe;
    busMessageEventData.writerPipe = nullptr;
    busMessageEventData.mainLoop = mainLoop;
    //to be notified of messages from videoCapturePipe, mostly EOS
    gstBus = gst_element_get_bus(videoCapturePipe);

    videoCaptureBusWatchId = gst_bus_add_watch(
        gstBus,
        (GstBusFunc)onCapturePipeMessage,
        &busMessageEventData);
    gst_object_unref(gstBus);

    //Activate new sample event in app sink pipe;
    programData.branchIndex = 1;
    captureOutGate = gst_bin_get_by_name(
        GST_BIN(videoCapturePipe),
        "videoCaptureSink");
    g_object_set(G_OBJECT(captureOutGate), "emit-signals", TRUE,
                 "sync", FALSE, NULL);
    g_signal_connect(
        captureOutGate,
        "new-sample",
        G_CALLBACK(onNewSampleFromCapture),
        &programData);

    gst_object_unref(captureOutGate);

    //Activate second branch event in app sink pipe
    programData2.branchIndex = 2;
    captureOutGate = gst_bin_get_by_name(
        GST_BIN(videoCapturePipe),
        "videoCaptureSink2");
    g_object_set(G_OBJECT(captureOutGate), "emit-signals", TRUE,
                 "sync", FALSE, NULL);
    g_signal_connect(
        captureOutGate,
        "new-sample",
        G_CALLBACK(onNewSampleFromCapture),
        &programData2);

    gst_object_unref(captureOutGate);

    //Run created pipes
    gst_element_set_state (videoCapturePipe, GST_STATE_PLAYING);

    //Run main loop by main thread to handle bus events.
    g_main_loop_run(mainLoop);

    qDebug() << "After mainloop terminate\n";

    gst_element_set_state (videoCapturePipe, GST_STATE_NULL);
    qDebug() << "After put capture pipe in null state\n";

    qDebug() << "Before terminating program\n";
    g_usleep(10000000);

    //free all allocated resources
    gst_object_unref(videoCapturePipe);
    g_main_loop_unref(mainLoop);
}
//****************************************************
static gboolean onCapturePipeMessage(
    GstBus *bus,
    GstMessage *message,
    gpointer data)
{
    bool result = true;

    BusMesageEventData *eventData = (BusMesageEventData *)data;
    switch (GST_MESSAGE_TYPE(message))
    {
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(message, &err, &debug);
        g_print ("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);
        gst_element_send_event(eventData->writerPipe, gst_event_new_eos());
        g_main_loop_quit(eventData->mainLoop);
        result = false;
        break;
    case GST_MESSAGE_EOS:
        qDebug() << "Got eos in capture bus\n";
        //gst_element_send_event(eventData->writerPipe, gst_event_new_eos());
        g_main_loop_quit(eventData->mainLoop);
        qDebug() << "Got eos in capture bus after quit main loop\n";
        result = false;
        break;
    case GST_MESSAGE_STATE_CHANGED:
        GstState old_state, new_state;

        gst_message_parse_state_changed(
            message,
            &old_state,
            &new_state, NULL);
        g_print ("******************Element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME (message->src),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
        break;
    default:
        /* unhandled message */
        break;
    }

    /* we want to be notified again the next time there is a message
   * on the bus, so returning true (false means we want to stop watching
   * for messages on the bus and our callback should not be called again)
   */
    return result;
}
////****************************************************
static int16_t onNewSampleFromCapture(GstElement *elt, gpointer data)
{
    GstData *gstData = (GstData *)data;
    static int counter1 = 0;
    //For second branch events
    static int counter2 = 0;

    int *counter = nullptr;
    if (1 == gstData->branchIndex)
    {
        counter = &counter1;
    }
    else
    {
        counter = &counter2;
    }
    //qDebug() << "New sample*************************\n";
    GstSample *sample;
    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample(GST_APP_SINK (elt));
    //Extract gst buffer of current sample
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if(*counter < 50)
    {
        //Extract buffer info
        GstMapInfo info;
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        //Extract buffer data and it's size
        guint8 *bufferData = info.data;
        gsize size = info.size;
        //Extract buffer meta data about current video frame
        int width = 0;
        int height = 0;
        GstStructure *capsStruct = nullptr;
        GstCaps *caps = gst_sample_get_caps(sample);
        capsStruct = gst_caps_get_structure(caps, 0);

        gst_structure_get_int(capsStruct, "width", &width);
        gst_structure_get_int(capsStruct, "height", &height);

        const gchar* name = gst_structure_get_name(capsStruct);
        const gchar* format = gst_structure_get_string(capsStruct, "format");

        qDebug() << "\n***********************************\n";
        qDebug() << "video format is: " << name << "\n";
        qDebug() << "Pixel format is: " << format << "\n";
        qDebug() << "Frame width is: " << width << "\n";
        qDebug() << "Frame height is: " << height << "\n";
        qDebug() << "Frame data size is: " << size << "\n";
        qDebug() << "Frame number is: " << *counter << "\n";
        qDebug() << "***********************************\n";

        //Save buffer data on file
        std::string fileName = "/home/nvidia/save_gstsample_data/" +
                               std::to_string(gstData->branchIndex) + "/" +
                               std::to_string(*counter) + ".dat";
        std::ofstream wf(fileName, std::ios::out | std::ios::binary);
        if(!wf)
        {
            qDebug() << "Cannot open file!\n";
        }
        else
        {
            wf.write((char *)bufferData, size);
            wf.close();
        }
        (*counter)++;
    }
    gst_sample_unref(sample);
    return 0;
}
