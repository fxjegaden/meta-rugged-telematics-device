#ifndef __LIBXML2__H
#define __LIBXML2__H 
#ifdef __cplusplus
extern "C" {
#endif

int get_xml_content(char *filename, char *parent_node,char *name, char *value);

int set_xml_content(char *filename, char *parent_node,char *name, char *value);

#define SRC_XML_FILE "/sample.xml" 
#ifdef __cplusplus
}
#endif
#endif /*__LIBXML2__H*/
