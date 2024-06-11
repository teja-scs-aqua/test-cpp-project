#ifndef K3DSDK_SERIALIZATION_XML_H
#define K3DSDK_SERIALIZATION_XML_H

#include <k3dsdk/ipersistent.h>

namespace k3d
{

class idocument;
class inode;
class mesh;

namespace selection { class set; }

namespace xml
{

class element;

/// Modifies an XML document as-needed so that both legacy and recent documents can be loaded with the same code
void upgrade_document(element& XML);

/// Serializes a document node to XML
void save(inode& Node, element& XML, const ipersistent::save_context& Context);
/// Loads a document node from XML
void load(inode& Node, element& XML, const ipersistent::load_context& Context);

/// Serializes a document pipeline to XML
void save_pipeline(idocument& Document, element& XML, const ipersistent::save_context& Context);
/// Loads a document pipeline from XML
void load_pipeline(idocument& Document, element& XML, const ipersistent::load_context& Context);

/// Serializes a mesh to XML 
void save(const mesh& Mesh, element& Container, const ipersistent::save_context& Context);
/// Loads a mesh from XML 
void load(mesh& Mesh, element& Container, const ipersistent::load_context& Context);

/// Serializes a selection to XML 
void save(const selection::set& Selection, element& Container, const ipersistent::save_context& Context);
/// Loads a selection from XML 
void load(selection::set& Selection, element& Container, const ipersistent::load_context& Context);

} // namespace xml

} // namespace k3d

#endif // !K3DSDK_SERIALIZATION_XML_H

