#include "TYApi.h"
#include "../sample/common/Utils.hpp"

#include <vector>

typedef enum PERCIPIO_STREAM_LIST {
  PERCIPIO_STREAM_COLOR     = 0x00000001,
  PERCIPIO_STREAM_DEPTH     = 0x00000002,
  PERCIPIO_STREAM_IR_LEFT   = 0x00000004,
  PERCIPIO_STREAM_IR_RIGHT  = 0x00000008,
}PERCIPIO_STREAM_LIST;
typedef int PERCIPIO_STREAM_ID;

typedef struct image_data {
  PERCIPIO_STREAM_ID streamID;

  int           timestamp;      ///< Timestamp in microseconds
  int           imageIndex;     ///< image index used in trigger mode
  int           status;         ///< Status of this buffer
  int           size;           ///< Buffer size
  void*         buffer;         ///< Pointer to data buffer
  int           width;          ///< Image width in pixels
  int           height;         ///< Image height in pixels
  int           pixelFormat;    ///< Pixel format, see TY_PIXEL_FORMAT_LIST

  image_data() {
    memset(this, 0, sizeof(image_data));
  }

  image_data(const TY_IMAGE_DATA& image) {
    timestamp = image.timestamp;
    imageIndex = image.imageIndex;
    status = image.status;
    size = image.size;
  
    width = image.width;
    height = image.height;
    pixelFormat =image.pixelFormat;

    switch(image.componentID) {
      case TY_COMPONENT_RGB_CAM:
        streamID = PERCIPIO_STREAM_COLOR;
        break;
      case TY_COMPONENT_DEPTH_CAM:
        streamID = PERCIPIO_STREAM_DEPTH;
        break;
      case TY_COMPONENT_IR_CAM_LEFT:
        streamID = PERCIPIO_STREAM_IR_LEFT;
        break;
      case TY_COMPONENT_IR_CAM_RIGHT:
        streamID = PERCIPIO_STREAM_IR_RIGHT;
        break;
      default:
        streamID = PERCIPIO_STREAM_COLOR;
        break;
    }

    if(size) {
      buffer = new char[size];
      memcpy(buffer, image.buffer, size);
    } else {
      buffer = NULL;
    }
  }

  image_data(const image_data & d) {
    streamID = d.streamID;
    timestamp = d.timestamp;
    imageIndex = d.imageIndex;
    status = d.status;
    size = d.size;
  
    width = d.width;
    height = d.height;
    pixelFormat =d.pixelFormat;

    if(size) {
      buffer = new char[size];
      memcpy(buffer, d.buffer, size);
    } else {
      buffer = NULL;
    }
  }

  ~image_data() {
    if(buffer) {
      delete []buffer;
      buffer = NULL;
    }
  }

  void* Ptr() {
    return buffer;
  }

}image_data;

struct PercipioDeviceEvent {
  virtual int run(TY_DEV_HANDLE handle, TY_EVENT event_id) = 0;
  virtual ~PercipioDeviceEvent() {}
};

static PercipioDeviceEvent *handler_ptr = NULL;
static int handler_execute(TY_DEV_HANDLE handle, TY_EVENT event_id) {
  // Make the call up to the target language when handler_ptr
  // is an instance of a target language director class
  return handler_ptr->run(handle, event_id);
}

void user_device_event_callback(TY_DEV_HANDLE handle, TY_EVENT_INFO evt) {
  handler_execute(handle, evt.eventId);
}

void percipio_device_callback(TY_EVENT_INFO *event_info, void *userdata) {
    if (event_info->eventId == TY_EVENT_DEVICE_OFFLINE) {
        // Note: 
        //     Please set TY_BOOL_KEEP_ALIVE_ONOFF feature to false if you need to debug with breakpoint!
        TY_DEV_HANDLE handle = (TY_DEV_HANDLE)userdata;
        TY_EVENT_INFO _event = *event_info;
        user_device_event_callback(handle, _event);
    }
    else if (event_info->eventId == TY_EVENT_LICENSE_ERROR) {
        LOGD("=== Event Callback: License Error!");
    }
}

class PercipioSDK
{
  public:
    PercipioSDK();
    ~PercipioSDK();

    TY_DEV_HANDLE Open();
    TY_DEV_HANDLE Open(const char* sn);
    TY_DEV_HANDLE OpenDeviceByIP(const char* ip);
        bool isValidHandle(const TY_DEV_HANDLE handle);
    void Close(const TY_DEV_HANDLE handle);

    //bool DeviceRegisterEventCallBack(const TY_DEV_HANDLE handle, const TY_EVENT_CALLBACK cbPtr);
    bool PercipioDeviceRegiststerCallBackEvent(PercipioDeviceEvent *handler);

    /**/
    bool                                DeviceStreamEnable(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    bool                                DeviceStreamDisable(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    const std::vector<TY_ENUM_ENTRY>&   DeviceStreamFormatDump(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    
    bool                                DeviceStreamFormatConfig(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream, const TY_ENUM_ENTRY fmt);
        int  Width(const TY_ENUM_ENTRY fmt);
        int  Height(const TY_ENUM_ENTRY fmt);



    const TY_CAMERA_CALIB_INFO& DeviceReadCalibData(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    int                         DeviceReadCalibWidth(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    int                         DeviceReadCalibHeight(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    
    const std::vector<float>   DeviceReadCalibIntrinsicArray(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    const std::vector<float>   DeviceReadCalibExtrinsicArray(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);
    const std::vector<float>   DeviceReadCalibDistortionArray(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream);

    bool  DeviceStreamOn(const TY_DEV_HANDLE handle);
    const std::vector<image_data>& DeviceStreamFetch(const TY_DEV_HANDLE handle, int timeout);
    bool DeviceStreamOff(const TY_DEV_HANDLE handle);

  private:
    typedef enum STREAM_FMT_LIST_IDX {
        STREMA_FMT_IDX_COLOR      = 0,
        STREMA_FMT_IDX_DEPTH      = 1,
        STREMA_FMT_IDX_IR_LEFT    = 2,
        STREMA_FMT_IDX_IR_RIGHT   = 3,
        STREMA_FMT_IDX_MAX        = 4,
      }STREAM_FMT_LIST_IDX;


    typedef struct device_info {
      TY_DEV_HANDLE       handle;
      std::string         devID;
      PERCIPIO_STREAM_ID  stream;
      std::vector<char>   frameBuffer[2];

      std::vector<TY_ENUM_ENTRY> fmt_list[STREMA_FMT_IDX_MAX];
      std::vector<image_data>    image_list;
      TY_CAMERA_CALIB_INFO       calib_data_list[STREMA_FMT_IDX_MAX];

      device_info(const TY_DEV_HANDLE _handle, const char* id) {
        frameBuffer[0].clear();
        frameBuffer[1].clear();
        for(size_t i = 0; i < STREMA_FMT_IDX_MAX; i++) {
          fmt_list[i].clear();
          memset(&calib_data_list[i], 0, sizeof(TY_CAMERA_CALIB_INFO));
        }
        handle = _handle;
        devID = std::string(id);
        image_list.clear();
      }

      ~device_info() {
      }

    }device_info;

    std::vector< TY_INTERFACE_HANDLE>  iFaceList;
    void AddInterface(const TY_INTERFACE_HANDLE iface);
    void AddDevice(const TY_DEV_HANDLE hanlde, const char* sn);

    std::vector<device_info>  DevList;

    int stream_idx(const PERCIPIO_STREAM_ID stream);
    
    int hasDevice(const TY_DEV_HANDLE handle);
    void DumpDeviceInfo(const TY_DEV_HANDLE handle);
    bool FrameBufferAlloc(TY_DEV_HANDLE handle, unsigned int frameSize);
    void FrameBufferRelease(TY_DEV_HANDLE handle) ;
};

void PercipioSDK::AddInterface(const TY_INTERFACE_HANDLE iface) {
    for (size_t i = 0; i < iFaceList.size(); i++) {
        if (iFaceList[i] == iface)
            return;
    }
    iFaceList.push_back(iface);
}

void PercipioSDK::AddDevice(const TY_DEV_HANDLE hanlde, const char* sn) {
	DevList.push_back(device_info(hanlde, sn));
}

int PercipioSDK::stream_idx(const PERCIPIO_STREAM_ID stream)
{
  int compIDX;
  switch(stream) {
    case PERCIPIO_STREAM_COLOR:
      compIDX = STREMA_FMT_IDX_COLOR;
      break;
    case PERCIPIO_STREAM_DEPTH:
      compIDX = STREMA_FMT_IDX_DEPTH;
      break;
    case PERCIPIO_STREAM_IR_LEFT:
      compIDX = STREMA_FMT_IDX_IR_LEFT;
      break;
    case PERCIPIO_STREAM_IR_RIGHT:
      compIDX = STREMA_FMT_IDX_IR_RIGHT;
      break;
    default:
      LOGE("stream mode not support : %d", stream);
      return STREMA_FMT_IDX_MAX;
  }
  return compIDX;
}

PercipioSDK::PercipioSDK() {
  TYInitLib();
}

PercipioSDK::~PercipioSDK() {
  for (size_t i = 0; i < iFaceList.size(); i++) {
      TYCloseInterface(iFaceList[i]);
  }
  TYDeinitLib();
}

TY_DEV_HANDLE PercipioSDK::Open(const char* sn) {
  std::string SN;
  if(sn != NULL)
    SN = std::string(sn);
  else
    SN = std::string("");
  std::vector<TY_DEVICE_BASE_INFO> selected;
  TY_STATUS status = selectDevice(TY_INTERFACE_ALL, SN, "", 10, selected);
  if(status != TY_STATUS_OK) {
    return 0;
  }

  if(!selected.size()) {
    return 0;
  }
  
  TY_INTERFACE_HANDLE hIface = NULL;
  TY_DEV_HANDLE hDevice = NULL;

  TY_DEVICE_BASE_INFO& selectedDev = selected[0];
  status = TYOpenInterface(selectedDev.iface.id, &hIface);
  if(status != TY_STATUS_OK) {
    LOGE("TYOpenInterface failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return 0;
  }

  status = TYOpenDevice(hIface, selectedDev.id, &hDevice);
  if(status != TY_STATUS_OK) {
    LOGE("TYOpenDevice failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return 0;
  }

  AddInterface(hIface);
  AddDevice(hDevice, selectedDev.id);
  LOGD("Device %s is on!", selectedDev.id);

  TYRegisterEventCallback(hDevice, percipio_device_callback, hDevice);

  DumpDeviceInfo(hDevice);

  return hDevice;
}

TY_DEV_HANDLE PercipioSDK::OpenDeviceByIP(const char* ip) {
  std::vector<TY_DEVICE_BASE_INFO> selected;
  TY_STATUS status = selectDevice(TY_INTERFACE_ALL, "", ip, 10, selected);
  if(status != TY_STATUS_OK) {
    return 0;
  }

  if(!selected.size()) {
    return 0;
  }
  
  TY_INTERFACE_HANDLE hIface = NULL;
  TY_DEV_HANDLE hDevice = NULL;

  TY_DEVICE_BASE_INFO& selectedDev = selected[0];
  status = TYOpenInterface(selectedDev.iface.id, &hIface);
  if(status != TY_STATUS_OK) {
    LOGE("TYOpenInterface failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return 0;
  }

  status = TYOpenDevice(hIface, selectedDev.id, &hDevice);
  if(status != TY_STATUS_OK) {
    LOGE("TYOpenDevice failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return 0;
  }

  DevList.push_back(device_info(hDevice, selectedDev.id));
  LOGD("Device %s is on!", selectedDev.id);

  DumpDeviceInfo(hDevice);

  return hDevice;
}

TY_DEV_HANDLE PercipioSDK::Open() {
  return Open(NULL);
}

int PercipioSDK::hasDevice(const TY_DEV_HANDLE handle) {
  for(size_t i = 0; i < DevList.size(); i++) {
    if(handle == DevList[i].handle)
      return i;
  }
  return -1;
}

bool PercipioSDK::isValidHandle(const TY_DEV_HANDLE handle) {
  if(handle && (hasDevice(handle) >= 0))
      return true;

  return false;
}

void PercipioSDK::DumpDeviceInfo(const TY_DEV_HANDLE handle) {
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("DumpDeviceInfo failed: invalid handle %s:%d", __FILE__, __LINE__);
    return ;
  }

  //DUMP STREAM FMT LIST
  static TY_COMPONENT_ID compID[STREMA_FMT_IDX_MAX] = {
      TY_COMPONENT_RGB_CAM,
      TY_COMPONENT_DEPTH_CAM,
      TY_COMPONENT_IR_CAM_LEFT,
      TY_COMPONENT_IR_CAM_RIGHT
  };

  TY_STATUS status;
  for(size_t cnt = 0; cnt < STREMA_FMT_IDX_MAX; cnt++) {
    unsigned int n = 0;
    status = TYGetEnumEntryCount(handle, compID[cnt], TY_ENUM_IMAGE_MODE, &n);
    if(status != TY_STATUS_OK) {
      LOGE("TYGetEnumEntryCount failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
      DevList[idx].fmt_list[cnt].clear();
      continue;
    }

    if (n == 0){
      DevList[idx].fmt_list[cnt].clear();
      continue;
    }

    DevList[idx].fmt_list[cnt].resize(n);

    TY_ENUM_ENTRY* pEntry = &DevList[idx].fmt_list[cnt][0];
    status = TYGetEnumEntryInfo(handle, compID[cnt], TY_ENUM_IMAGE_MODE, pEntry, n, &n);
    if(status != TY_STATUS_OK) {
      LOGE("TYGetEnumEntryInfo failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
      DevList[idx].fmt_list[cnt].clear();
    }
  }

  //DUMP STREAM CALIB_DATA LIST
  for(size_t cnt = 0; cnt < STREMA_FMT_IDX_MAX; cnt++) {
    status = TYGetStruct(handle, compID[cnt], TY_STRUCT_CAM_CALIB_DATA, &DevList[idx].calib_data_list[cnt], sizeof(TY_CAMERA_CALIB_INFO));
    if(status != TY_STATUS_OK) {
      LOGE("TYGetStruct failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
      memset(&DevList[idx].calib_data_list[cnt], 0, sizeof(TY_CAMERA_CALIB_INFO));
    }
  }
}
#if 0
bool PercipioSDK::DeviceRegisterEventCallBack(const TY_DEV_HANDLE handle, const TY_EVENT_CALLBACK cbPtr) {
  if(hasDevice(handle) < 0) {
    LOGE("DeviceRegisterEventCallBack failed: invalid handle %s:%d", __FILE__, __LINE__);
    return false;
  }

  TY_STATUS status = TYRegisterEventCallback(handle, cbPtr, NULL);
  if(status != TY_STATUS_OK) {
    LOGE("TYRegisterEventCallback failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return false;
  }
  return true;
}
#else
bool PercipioSDK::PercipioDeviceRegiststerCallBackEvent(PercipioDeviceEvent *handler) {
  handler_ptr = handler;
  handler = NULL;
  return true;
}
#endif
bool PercipioSDK::DeviceStreamEnable(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream) {
  TY_STATUS status;
  TY_COMPONENT_ID allComps;
  status = TYGetComponentIDs(handle, &allComps);
  if(status != TY_STATUS_OK) {
    LOGE("TYGetComponentIDs failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return false;
  }

  if(stream & PERCIPIO_STREAM_COLOR) {
    if(allComps & TY_COMPONENT_RGB_CAM) {
      status = TYEnableComponents(handle, TY_COMPONENT_RGB_CAM);
      if(status != TY_STATUS_OK) {
        LOGE("TYEnableComponents failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
        return false;
      }
    } else {
      LOGE("The device does not support color stream.\n");
      return false;
    }
  } else {
    TYDisableComponents(handle, TY_COMPONENT_RGB_CAM);
  }

  if(stream & PERCIPIO_STREAM_DEPTH) {
    if(allComps & TY_COMPONENT_DEPTH_CAM) {
      status = TYEnableComponents(handle, TY_COMPONENT_DEPTH_CAM);
      if(status != TY_STATUS_OK) {
        LOGE("TYEnableComponents failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
        return false;
      }
    } else {
      LOGE("The device does not support depth stream.\n");
      return false;
    }
  } else {
    TYDisableComponents(handle, TY_COMPONENT_DEPTH_CAM);
  }

  if(stream & PERCIPIO_STREAM_IR_LEFT) {
    if(allComps & TY_COMPONENT_IR_CAM_LEFT) {
      status = TYEnableComponents(handle, TY_COMPONENT_IR_CAM_LEFT);
      if(status != TY_STATUS_OK) {
        LOGE("TYEnableComponents failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
        return false;
      }
    } else {
      LOGE("The device does not support left ir stream.\n");
      return false;
    }
  } else {
    TYDisableComponents(handle, TY_COMPONENT_IR_CAM_LEFT);
  }

  if(stream & PERCIPIO_STREAM_IR_RIGHT) {
    if(allComps & TY_COMPONENT_IR_CAM_RIGHT) {
      status = TYEnableComponents(handle, TY_COMPONENT_IR_CAM_RIGHT);
      if(status != TY_STATUS_OK) {
        LOGE("TYEnableComponents failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
        return false;
      }
    } else {
      LOGE("The device does not support right ir stream.\n");
      return false;
    }
  } else {
    TYDisableComponents(handle, TY_COMPONENT_IR_CAM_RIGHT);
  }

  return true;
}

bool PercipioSDK::DeviceStreamDisable(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  if(stream & PERCIPIO_STREAM_COLOR) {
    TYDisableComponents(handle, TY_COMPONENT_RGB_CAM);
  }

  if(stream & PERCIPIO_STREAM_DEPTH) {
    TYDisableComponents(handle, TY_COMPONENT_DEPTH_CAM);
  }

  if(stream & PERCIPIO_STREAM_IR_LEFT) {
    TYDisableComponents(handle, TY_COMPONENT_IR_CAM_LEFT);
  }

  if(stream & PERCIPIO_STREAM_IR_RIGHT) {
    TYDisableComponents(handle, TY_COMPONENT_IR_CAM_RIGHT);
  }
  return true;
}

const std::vector<TY_ENUM_ENTRY>& PercipioSDK::DeviceStreamFormatDump(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream) {
  static std::vector<TY_ENUM_ENTRY>  invalid_enum;
  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return invalid_enum;
  }

  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return invalid_enum;
  }

  return DevList[idx].fmt_list[compIDX];
}

int  PercipioSDK::Width(const TY_ENUM_ENTRY fmt) {
  return TYImageWidth(fmt.value);
}
int  PercipioSDK::Height(const TY_ENUM_ENTRY fmt) {
  return TYImageHeight(fmt.value);
}

bool PercipioSDK::DeviceStreamFormatConfig(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream, const TY_ENUM_ENTRY fmt) {
  TY_COMPONENT_ID compID;
  switch(stream) {
    case PERCIPIO_STREAM_COLOR:
      compID = TY_COMPONENT_RGB_CAM;
      break;
    case PERCIPIO_STREAM_DEPTH:
      compID = TY_COMPONENT_DEPTH_CAM;
      break;
    case PERCIPIO_STREAM_IR_LEFT:
      compID = TY_COMPONENT_IR_CAM_LEFT;
      break;
    case PERCIPIO_STREAM_IR_RIGHT:
      compID = TY_COMPONENT_IR_CAM_RIGHT;
      break;
    default:
      LOGE("stream mode not support : %d", stream);
      return false;
  }

  TY_STATUS status = TYSetEnum(handle, compID, TY_ENUM_IMAGE_MODE, fmt.value);
  if(status != TY_STATUS_OK) {
    LOGE("Stream fmt is not support!");
    return false;
  }

  return true;
}

bool PercipioSDK::FrameBufferAlloc(TY_DEV_HANDLE handle, unsigned int frameSize) {
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return false;
  }

  DevList[idx].frameBuffer[0].resize(frameSize);
  DevList[idx].frameBuffer[1].resize(frameSize);

  void* pBuf1 = &DevList[idx].frameBuffer[0][0];
  void* pBuf2 = &DevList[idx].frameBuffer[1][0];
  ASSERT_OK(TYEnqueueBuffer(handle, pBuf1, frameSize));
  ASSERT_OK(TYEnqueueBuffer(handle, pBuf2, frameSize));

  return true;
}

void PercipioSDK::FrameBufferRelease(TY_DEV_HANDLE handle) {
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return ;
  }

  TYClearBufferQueue(handle);
  DevList[idx].frameBuffer[0].clear();
  DevList[idx].frameBuffer[1].clear();
  return ;
}

const TY_CAMERA_CALIB_INFO& PercipioSDK::DeviceReadCalibData(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  static TY_CAMERA_CALIB_INFO  invalid_calib_data;
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return invalid_calib_data;
  }

  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return invalid_calib_data;
  }

  return DevList[idx].calib_data_list[compIDX];
}

int PercipioSDK::DeviceReadCalibWidth(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return 0;
  }

  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return 0;
  }

  return DevList[idx].calib_data_list[compIDX].intrinsicWidth;
}

int PercipioSDK::DeviceReadCalibHeight(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return 0;
  }

  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return 0;
  }

  return DevList[idx].calib_data_list[compIDX].intrinsicHeight;
}

const std::vector<float> PercipioSDK::DeviceReadCalibIntrinsicArray(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return std::vector<float>();
  }

  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return std::vector<float>();
  }

  float* ptr = DevList[idx].calib_data_list[compIDX].intrinsic.data;
  int cnt = sizeof(DevList[idx].calib_data_list[compIDX].intrinsic.data) / sizeof(float);
  return std::vector<float>(ptr, ptr + cnt);
}

const std::vector<float> PercipioSDK::DeviceReadCalibExtrinsicArray(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return std::vector<float>();
  }

  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return std::vector<float>();
  }

  float* ptr = DevList[idx].calib_data_list[compIDX].extrinsic.data;
  int cnt = sizeof(DevList[idx].calib_data_list[compIDX].extrinsic.data) / sizeof(float);
  return std::vector<float>(ptr, ptr + cnt);
}

const std::vector<float> PercipioSDK::DeviceReadCalibDistortionArray(const TY_DEV_HANDLE handle, const PERCIPIO_STREAM_ID stream)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return std::vector<float>();
  }

  int compIDX = stream_idx(stream);
  if(compIDX == STREMA_FMT_IDX_MAX) {
      return std::vector<float>();
  }

  float* ptr = DevList[idx].calib_data_list[compIDX].distortion.data;
  int cnt = sizeof(DevList[idx].calib_data_list[compIDX].distortion.data) / sizeof(float);
  return std::vector<float>(ptr, ptr + cnt);
}

bool PercipioSDK::DeviceStreamOn(const TY_DEV_HANDLE handle)
{
  if(hasDevice(handle) < 0) {
    LOGE("Invalid device handle!");
    return false;
  }

  unsigned int frameSize;
  TY_STATUS status = TYGetFrameBufferSize(handle, &frameSize);
  if(status != TY_STATUS_OK) {
    LOGE("TYGetFrameBufferSize failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return false;
  }

  if(!FrameBufferAlloc(handle, frameSize)) {
    return false;
  }

  status = TYStartCapture(handle);
  if(status != TY_STATUS_OK) {
    LOGE("TYStartCapture failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return false;
  }

  LOGD("Stream ON!");
  return true;
}

const std::vector<image_data>& PercipioSDK::DeviceStreamFetch(const TY_DEV_HANDLE handle, int timeout) {
  static std::vector<image_data> INVALID_FRAME(0);
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return INVALID_FRAME;
  }

  TY_FRAME_DATA frame;
  DevList[idx].image_list.clear();
  TY_STATUS status = TYFetchFrame(handle, &frame, timeout);
  if(status != TY_STATUS_OK) {
    LOGE("TYFetchFrame failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return INVALID_FRAME;
  }

  for (int i = 0; i < frame.validCount; i++) {
    if (frame.image[i].status != TY_STATUS_OK) continue;

    // get depth image
    if (frame.image[i].componentID == TY_COMPONENT_DEPTH_CAM){
      DevList[idx].image_list.push_back(image_data(frame.image[i]));
    }
    
    // get left ir image
    if (frame.image[i].componentID == TY_COMPONENT_IR_CAM_LEFT) {
      DevList[idx].image_list.push_back(image_data(frame.image[i]));
    }
    
    // get right ir image
    if (frame.image[i].componentID == TY_COMPONENT_IR_CAM_RIGHT) {
      DevList[idx].image_list.push_back(image_data(frame.image[i]));
    }

    // get BGR
    if (frame.image[i].componentID == TY_COMPONENT_RGB_CAM) {
      DevList[idx].image_list.push_back(image_data(frame.image[i]));
    }
  }

  status = TYEnqueueBuffer(handle, frame.userBuffer, frame.bufferSize);
  if(status != TY_STATUS_OK) {
    LOGE("TYEnqueueBuffer failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
  }
  return DevList[idx].image_list;
}


bool PercipioSDK::DeviceStreamOff(const TY_DEV_HANDLE handle)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return false;
  }


  TY_STATUS status = TYStopCapture(handle);
  if(status != TY_STATUS_OK) {
    LOGE("TYStopCapture failed: error %d(%s) at %s:%d", status, TYErrorString(status), __FILE__, __LINE__);
    return false;
  }

  LOGD("Stream OFF!");
  FrameBufferRelease(handle);

  return true;
}

void PercipioSDK::Close(const TY_DEV_HANDLE handle)
{
  int idx = hasDevice(handle);
  if(idx < 0) {
    LOGE("Invalid device handle!");
    return ;
  }

  LOGD("Device %s is close!", DevList[idx].devID.c_str());
  DevList.erase(DevList.begin() + idx);
  
  TYCloseDevice(handle);
}