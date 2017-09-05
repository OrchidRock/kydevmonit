#include "interlayer.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
//#include <libxml2/libxml/parser.h>
//#include <libxml2/libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define WORK_ROOT_DIR "/etc/kydevmonit/"
typedef void (*NO_DBUS_INPUT_FUNC)(void* return_t);
typedef void (*HAS_DBUS_INPUT_FUNC)(const char* input, void* return_t);
typedef struct{
    int error_no;
    int count;
    char * desc;
}hw_return_t;

#define NO_DBUS_INPUT_TYPE 0
#define HAS_DBUS_INPUT_TYPE 1
typedef struct{
    void * address;
    int type;    
}device_method_t;


static xmlDocPtr intropectdoc;
static xmlNodePtr intropectdoc_rootnode;
static int
_init_write_devices_intropect_xml(const char* object_name){
    if(intropectdoc == NULL && intropectdoc_rootnode == NULL)    {
        intropectdoc = xmlNewDoc("1.0");
        intropectdoc_rootnode = xmlNewNode(NULL, "object");
        xmlNewProp(intropectdoc_rootnode,"name", object_name);
        xmlDocSetRootElement(intropectdoc, intropectdoc_rootnode);
    }
    return 0;
}
static int
_end_write_devices_intropect_xml(const char* dbus_name){
    if(intropectdoc == NULL || intropectdoc_rootnode == NULL)
        return -1;
    char filename[1024]={0};
    sprintf(filename,"%sdevices.", WORK_ROOT_DIR);
    sprintf(filename, "%s%s.xml", filename,dbus_name);
    printf("intropect xml filename: %s\n", filename);
    if(xmlSaveFormatFileEnc(filename, intropectdoc, "UTF-8", 1) >=0){
        printf("Save intropectdoc xml file successful\n");
    }else{
        printf("Save intropectdoc xml file failed\n");
    }
    xmlFreeDoc(intropectdoc);
    xmlCleanupParser();
    return 0;
}
static int
_add_device_to_intropect_xml(const xmlChar* devicename,const char* devicecode){
    if(intropectdoc == NULL || intropectdoc_rootnode == NULL || devicename == NULL){
        fprintf(stderr, "doc or node or devicename is null\n");
        return -1;
    }
    xmlNodePtr devicenode = xmlNewChild(intropectdoc_rootnode, NULL, "device", "");
    xmlNewProp(devicenode, "name", devicename);
    xmlNewProp(devicenode, "code",devicecode);
    return 0;
}

static GHashTable * method_table = NULL;
static int
_init_method_hashtable(){
    if(method_table == NULL){
        method_table = g_hash_table_new(g_str_hash, g_str_equal);
    }
    return 0; 
}
static int
_insert_method_hashtable(gpointer key, gpointer value){
    if(method_table == NULL || key == NULL)
        return -1;
    g_hash_table_insert(method_table, key, value);
    return 0;
}
static gpointer
_get_method_address(gpointer key){
    if(method_table == NULL || key == NULL){
        //fprintf(stderr, "ERROR: method_table or key is null\n");
        return NULL;
    }
    return g_hash_table_lookup(method_table,key);
}
static void
_ms_print_key_value(gpointer key, gpointer value, gpointer user_data){
    printf("key: %s, value: %u\n", key, (unsigned int)value);
}
static void
_ms_display_method_hashtable(){
    g_hash_table_foreach(method_table, _ms_print_key_value, NULL);
}
static int
_parse_device_xml(const char * filename, const char* object_name){
    //if(filename == NULL)
      //  return -1;
    char filepath[1024]={0};
    sprintf(filepath,"%s",WORK_ROOT_DIR);
    sprintf(filepath,"%s%s",filepath,filename);
    xmlNodePtr curnode;
    xmlDocPtr doc;
    void * sharelibHandle;
    xmlInitParser();
    doc = xmlReadFile(filepath, "UTF-8", XML_PARSE_RECOVER) ;
    if(doc == NULL){
        fprintf(stderr, "xmlReadFile %s failed\n", filepath);
        return -1;
    }
    curnode = xmlDocGetRootElement(doc);
    if(curnode == NULL){
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        return -1;
    }
   // printf("curnode->name: %s\n", curnode->name);
   // curnode = curnode->xmlChildrenNode;
    xmlChar * nodevalue = NULL;
    xmlChar * devicecode = NULL;
    xmlChar * methodcode = NULL;
    xmlChar * devicepath = NULL;
    int statuscode = 0;
    while(curnode != NULL){
        if(xmlStrcmp(curnode->name, (const xmlChar*)"object") == 0){
            nodevalue = xmlGetProp(curnode, (const xmlChar*)"name");
            //printf("nodevalue: %s\n", nodevalue);
            if(xmlStrncmp(nodevalue, object_name, strlen(object_name)) != 0){
               fprintf(stderr,"The object_name %s is not matched\n", nodevalue);
               xmlFree(nodevalue);
               statuscode = -1;
               break;
            }else{
                _init_write_devices_intropect_xml(object_name);
                xmlFree(nodevalue);
                curnode = curnode->xmlChildrenNode;
                continue;
            }
        }
        else if(xmlStrcmp(curnode->name, (const xmlChar*)"device") == 0){
            nodevalue = xmlGetProp(curnode, "name");
            if(nodevalue == NULL){
                fprintf(stderr, "The device hasn't name,this is invailed!\n");
                statuscode = -1;
                break;
            }
            devicecode = xmlGetProp(curnode, "code");
            if(devicecode == NULL){
                fprintf(stderr, "The device hasn't code,this is invailed!\n");
                xmlFree(nodevalue);
                statuscode = -1;
                break;
            }
            // load so
            devicepath = xmlGetProp(curnode, "path");
            if(devicepath == NULL){
                fprintf(stderr, "The device hasn't path,this is invailed!\n");
                statuscode = -1;
                xmlFree(nodevalue);
                xmlFree(devicecode); 
                break; 
            }
            printf("path: %s\n", devicepath);
            sharelibHandle = dlopen(devicepath, RTLD_NOW); 
            if(sharelibHandle == NULL){
                fprintf(stderr, "%s sharelibHandle is NULL\n", devicepath);
                xmlFree(devicepath);
                xmlFree(nodevalue);
                xmlFree(devicecode); 
                statuscode = -1;
                break;
            }else{
                _add_device_to_intropect_xml(nodevalue, devicecode);
                _init_method_hashtable();
                xmlFree(nodevalue);
                xmlFree(devicepath);
            }
            curnode = curnode->xmlChildrenNode;
            continue;
        }else if(xmlStrcmp(curnode->name, (const xmlChar*)"method") == 0){
            nodevalue = xmlGetProp(curnode, "name");
            if(nodevalue == NULL){
                fprintf(stderr, "method hasn't name, this is invaild!\n");
                curnode = curnode->next;
                continue;
            } 
            methodcode = xmlGetProp(curnode, "code");
            if(methodcode == NULL){
                fprintf(stderr, "method %s hasn't code, this is invaild!\n", nodevalue);
                curnode = curnode->next;
                xmlFree(nodevalue);
                continue;
            } 
            void* methodaddr = dlsym(sharelibHandle,nodevalue);
            char* key = malloc(20*sizeof(char));
            device_method_t* method_ptr = malloc(sizeof(device_method_t));
            method_ptr->address = methodaddr;
            xmlFree(nodevalue);

            nodevalue = xmlGetProp(curnode, "type");
            if(nodevalue == NULL){
                fprintf(stderr, "method hasn't type, this is invaild!\n");
                xmlFree(methodcode);
                curnode = curnode->next;
                continue;
            }
            else if(xmlStrcmp(nodevalue, "nodbusinput") == 0){
                method_ptr->type = NO_DBUS_INPUT_TYPE;
            }else if(xmlStrcmp(nodevalue, "hasdbusinput") == 0){
                method_ptr->type = HAS_DBUS_INPUT_TYPE;
            }else{
            
            }

            sprintf(key, "%s%s", devicecode,methodcode);
            printf("key: %s len=%lu\n", key, strlen(key));
            _insert_method_hashtable(key, method_ptr);

            printf("method name : %s, address : %u\n", nodevalue, (unsigned int)methodaddr);
            xmlFree(nodevalue);
            xmlFree(methodcode);
        }
        curnode = curnode->next;
    }
    if(devicecode != NULL)
        xmlFree(devicecode);
    if(doc != NULL)
        xmlFreeDoc(doc);
    
    return statuscode;
}
int interlayer_load_devices(const char* dbus_name, const char* object_name){
    if(dbus_name == NULL || object_name == NULL){
        fprintf(stderr, "ERROR: The dbus_name or object_name is null!\n");
        return -1;
    }
    // Loop match xml file
    DIR *workdp;
    struct dirent *entry;
    if((workdp = opendir(WORK_ROOT_DIR)) == NULL){ 
        fprintf(stderr, "opendir %s failed,no devices\n", WORK_ROOT_DIR);
        return -1;
    }
    //chdir(WORK_ROOT_DIR);
    while((entry = readdir(workdp)) != NULL){
        if(strncmp(dbus_name,entry->d_name, strlen(dbus_name)) == 0){
            
            if(_parse_device_xml(entry->d_name, object_name) < 0){
                fprintf(stderr, "%s parse failed!\n", entry->d_name);
            }
            else{
                printf("%s load successful\n", entry->d_name);
            }
        }
    }
    closedir(workdp);
    _end_write_devices_intropect_xml(dbus_name);
    //_ms_display_method_hashtable();
    return 0;   
}

int interlayer_get_devices_intropect(const char* dbus_name, const char* object_name, const char* devicename,char* ret_desc){
    if(dbus_name == NULL || object_name == NULL || ret_desc == NULL){
        fprintf(stderr, "ERROR: The dbus_name or object_name or ret_desc is null!\n");
        return -1;
    }
    char filename[1024];
    memset(filename, 0, 1024);
    size_t len=0;
    if(strlen(devicename) == 0){
        sprintf(filename,"%sdevices.%s.xml", WORK_ROOT_DIR, dbus_name);
        printf("filename= %s\n", filename);
        int fd=open(filename, O_RDONLY);
        if(fd < 0){
            fprintf(stderr, "%s open failed\n", filename);
            return -1;
        }
        
        if((len = read(fd, ret_desc, 1024-1)) < 0){
            fprintf(stderr, "%s read failed\n", filename);
            close(fd);
            return -1;
        }
        ret_desc[len] = '\0';
        //printf("ret_desc = %s\n", ret_desc);
        close(fd) ;
    }else{
        memset(filename, 0, 1024);
        sprintf(filename,"%s%s.%s.xml", WORK_ROOT_DIR, dbus_name,devicename);
        printf("filename= %s\n", filename);
        int fd=open(filename, O_RDONLY);
        if(fd < 0){
            fprintf(stderr, "%s open failed\n", filename);
            return -1;
        }
        
        if((len = read(fd, ret_desc, 1024-1)) < 0){
            fprintf(stderr, "%s read failed\n", filename);
            close(fd);
            return -1;
        }
        ret_desc[len] = '\0';
        //printf("ret_desc = %s\n", ret_desc);
        close(fd) ;
        
    }
    return 0;
}
int interlayer_call_device_method(int devicecode, int methodcode, const char* param, int* statuscode, int * count, char* desc){
    if(param == NULL || statuscode == NULL || count == NULL || desc == NULL){
        fprintf(stderr, "ERROR: Please input vaild paramters\n");
        return -1;
    }
    //_ms_display_method_hashtable();
    char key[20] = {0};
    sprintf(key,"0x%.2x0x%.2x", devicecode, methodcode);
    printf("call_key: %s ,len=%lu\n", key, strlen(key));
    device_method_t * method_ptr = _get_method_address(key);
    if(method_ptr == NULL){
        fprintf(stderr,"The method_ptr is NULL\n");
        return -1;
    }
    printf("method address : %u\n", (unsigned int)(method_ptr->address));
    
    hw_return_t result={0,0,NULL};
    result.desc = desc;
    
    if(method_ptr->type == NO_DBUS_INPUT_TYPE){
        NO_DBUS_INPUT_FUNC method_func = (NO_DBUS_INPUT_FUNC)(method_ptr->address);
        method_func(&result);
    }else if(method_ptr->type == HAS_DBUS_INPUT_TYPE){
        HAS_DBUS_INPUT_FUNC method_func = (HAS_DBUS_INPUT_FUNC)(method_ptr->address);
        method_func(param,&result);
    }else{
        //
    }
    *statuscode = result.error_no;
    *count = result.count;
    return 0;
}
