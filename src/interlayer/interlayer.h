#ifndef __INTER_LAYER_H__
#define __INTER_LAYER_H__


/**
 * The device's register info at /etc/kydevmonit/ xml file. 
 * This method find and load the device's xml file that match the
 * dbus_name and object_name. 
 * And load the share library of matched dervice.
 * @return 0 successful; 1 failed
 */
int interlayer_load_devices(const char* dbus_name, const char* object_name);

/**
 * Return the information to ret_desc about had registered device.
 * If the len(devicename) == 0 , the information only include device_name,device_code.
 * else the information include all method's name, method's code, method's type for the
 * special device by param 'devicename' point.
 *
 * @return 0 successful ; 1 failed
 */
int interlayer_get_devices_intropect(const char* dbus_name, const char* object_name, const char* devicename,char* ret_desc);

/**
  * Call the device method by devicecode and methodcode point.
  * @param If this device method need a param
  * @error_no the device method invoke return statuscode
  * @count
  * @desc The describtion about the error_no
  * @return 0 successful ; 1 failed
 */
int interlayer_call_device_method(int devicecode, int methodcode,const char* param, int* error_no, int* count, char* desc);

#endif
