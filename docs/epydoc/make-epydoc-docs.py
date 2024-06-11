#python

import k3d
import shutil

import epydoc.docbuilder
import epydoc.docwriter.html

# Use restructured text as the default for docstrings
#import epydoc.docstringparser
#epydoc.docstringparser.DEFAULT_DOCFORMAT = "restructuredtext"

index = epydoc.docbuilder.build_doc_index([k3d])
epydoc.docwriter.html.HTMLWriter(index).write("html")

# Overwrite the default CSS
shutil.copy("@epydoc_SOURCE_DIR@/epydoc.css", "@epydoc_BINARY_DIR@/html")

