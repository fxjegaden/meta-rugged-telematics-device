#ifndef __LIBXML2__H
#define __LIBXML2__H 


int get_xml_content(char *filename, char *parent_node,char *name, char *value);

int set_xml_content(char *filename, char *parent_node,char *name, char *value);

#define SRC_XML_FILE "/sample.xml" 
#endif /*__LIBXML2__H*/
