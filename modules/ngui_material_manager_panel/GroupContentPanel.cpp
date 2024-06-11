#include "GroupContentPanel.h"

namespace module
{
namespace ngui
{
namespace material_manager
{
namespace mechanics
{

GroupContentPanel::~GroupContentPanel()
{
  //Clean Up Dynamic Allocated Memory :)

  //All GTK Widgets >>

  //ALL Gtk Hboxes*****************
  std::vector<Gtk::HBox*>::iterator hbox_iter = m_pview_data_conts.begin();
  for(; hbox_iter != m_pview_data_conts.end(); ++hbox_iter)
    delete (*hbox_iter);  

  hbox_iter = m_label_data_conts.begin();
  for(; hbox_iter != m_label_data_conts.end(); ++hbox_iter)
    delete (*hbox_iter);  


  //ALL Gtk Vboxes*****************
  std::vector<Gtk::VBox*>::iterator vbox_iter = m_data_desc_conts.begin();
  for(; vbox_iter != m_data_desc_conts.end(); ++vbox_iter)
    delete (*vbox_iter); 
            
  vbox_iter = m_label_conts.begin();
  for(; vbox_iter != m_label_conts.end(); ++vbox_iter)
    delete (*vbox_iter); 

  vbox_iter = m_data_conts.begin();
  for(; vbox_iter != m_data_conts.end(); ++vbox_iter)
    delete (*vbox_iter); 


  //ALL Gtk Frames*****************
  std::vector<Gtk::Frame*>::iterator frame_iter = m_pview_frames.begin();
  for(; frame_iter != m_pview_frames.end(); ++frame_iter)
    delete (*frame_iter); 


  //ALL Gtk Labels*****************
  std::vector<Gtk::Label*>::iterator label_iter = m_name_labels.begin();
  for(; label_iter != m_name_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_type_labels.begin();
  for(; label_iter != m_type_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_datemod_labels.begin();
  for(; label_iter != m_datemod_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_artistname_labels.begin();
  for(; label_iter != m_artistname_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_name_data_labels.begin();
  for(; label_iter != m_name_data_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_type_data_labels.begin();
  for(; label_iter != m_type_data_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_datemod_data_labels.begin();
  for(; label_iter != m_datemod_data_labels.end(); ++label_iter)
    delete (*label_iter); 

  label_iter = m_artistname_data_labels.begin();
  for(; label_iter != m_artistname_data_labels.end(); ++label_iter)
    delete (*label_iter); 

            
  //ALL Gtk Textviews***************
  std::vector<Gtk::TextView*>::iterator mltxt_iter = m_artistnotes_mltext.begin();
  for(; mltxt_iter != m_artistnotes_mltext.end(); ++mltxt_iter)
    delete (*mltxt_iter); 


  //ALL Gtk ScrolledWindows*********
  std::vector<Gtk::ScrolledWindow*>::iterator scroll_iter = m_artistnotes_scrollwin.begin();
  for(; scroll_iter != m_artistnotes_scrollwin.end(); ++scroll_iter)
    delete (*scroll_iter); 


  //ALL Gtk HSeparator**************
  std::vector<Gtk::HSeparator*>::iterator hsep_iter = m_data_notes_seps.begin();
  for(; hsep_iter != m_data_notes_seps.end(); ++hsep_iter)
    delete (*hsep_iter); 

  hsep_iter = m_materials_seps.begin();
  for(; hsep_iter != m_materials_seps.end(); ++hsep_iter)
    delete (*hsep_iter); 


  //ALL Gtk VSeparator**************
  std::vector<Gtk::VSeparator*>::iterator vsep_iter = m_label_data_seps.begin();
  for(; vsep_iter !=  m_label_data_seps.end(); ++vsep_iter)
    delete (*vsep_iter); 


  //Disconect Signal Connections****
  m_timer_connection.disconnect();

}



void GroupContentPanel::init()
{
  //GLib Timer > Updates Preview Image Every 0.25s
  m_timer_connection = Glib::signal_timeout()
    .connect(sigc::mem_fun(*this, &GroupContentPanel::updatePreviewImage), 250);
}



void GroupContentPanel::buildPanel()
{
  if(m_hpane)
    {
      //Add Container To Right Pane From Implementation
      m_hpane->add2(m_toolbar_main_cont);

      //Embed The ToolBar
      m_toolbar_main_cont.pack_start(m_toolbar, false, false, 0);

      //----Insert Toolbar Setup HERE-----

      //Setup & Embed The Panel ScrollWindow
      m_scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      m_toolbar_main_cont.pack_start(m_scrolled_window, true, true, 0);
      m_scrolled_window.add(m_materials_cont);

      //Integer Value To Tack Onto Filenames To Make Them Unique (In This Panel)
      int fileName_int = 0;

      //For Each Of The Elements In Group->m_materials Create A GUI Strip
      std::list<MaterialObj*>::const_iterator mat_iter 
        = m_materialgrp->materialBegin();

      for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
        {
          //Create File Name For Material Preview Image
          k3d::string_t int_str;
          k3d::string_t finalFile_str;
          std::stringstream out;
          out << fileName_int;
          int_str = out.str();
          int_str += k3d::string_t(".png");
          finalFile_str.append(m_multi_imgfile);
          finalFile_str.append(int_str);		    

          //Create The Material Preview Image Object
          RenderedImage *s_preview_obj 
            = new RenderedImage(k3d::system::get_temp_directory() 
                                / k3d::filesystem::generic_path(finalFile_str));

          //Add Material Preview Image Object To List
          //m_material_pviews.push_back(s_preview_obj);
		    
          
          //Set The MaterialObj's Preview Image Object
          (*mat_iter)->setPviewImg(s_preview_obj);


          //Create & Embed Container To Hold A Single Material Section
          Gtk::HBox *t_HBox = new Gtk::HBox;
          m_pview_data_conts.push_back(t_HBox);
          m_materials_cont.pack_start(*t_HBox, false, false, 10);

          //Create & Embed The Preview Frame For Each Material
          Gtk::Frame *t_Frame = new Gtk::Frame("Preview Render:");
          t_Frame->add(*s_preview_obj);
          m_pview_frames.push_back(t_Frame);
          t_Frame->set_size_request(m_pview_size + 25, m_pview_size + 35);
          t_HBox->pack_start(*t_Frame, false, false, 5);

          //Create & Embed Data & Artist Notes Container
          Gtk::VBox *t_VBox_dd = new Gtk::VBox;
          m_data_desc_conts.push_back(t_VBox_dd);
          t_HBox->pack_start(*t_VBox_dd, true, true, 5); //HERE

          //Data Container (Contains x3 VBox -> Label, vertical break and Data)
          Gtk::HBox *t_HBox_ld = new Gtk::HBox;
          m_label_data_conts.push_back(t_HBox_ld);
          t_VBox_dd->pack_start(*t_HBox_ld, false, false, 5);
		    
          //Create & Embed The Label Container
          Gtk::VBox *t_VBox_label = new Gtk::VBox;
          m_label_conts.push_back(t_VBox_label);
          t_HBox_ld->pack_start(*t_VBox_label, false, false, 0);
          t_VBox_label->set_spacing(4);

          
          //Label (Non-Data) Creation
          Gtk::Label *t_Name_label = new Gtk::Label("Material Name:");
          Gtk::Label *t_Type_label = new Gtk::Label("Material Type:");
          Gtk::Label *t_dateMod_label = new Gtk::Label("Date Modified:");
          Gtk::Label *t_ArtistName_label = new Gtk::Label("Artist's Name:");

          m_name_labels.push_back(t_Name_label);
          m_type_labels.push_back(t_Type_label);
          m_datemod_labels.push_back(t_dateMod_label);
          m_artistname_labels.push_back(t_ArtistName_label);

          t_VBox_label->pack_start(*t_Name_label, false, false, 0);
          t_VBox_label->pack_start(*t_Type_label, false, false, 0);
          t_VBox_label->pack_start(*t_dateMod_label, false, false, 0);
          t_VBox_label->pack_start(*t_ArtistName_label, false, false, 0);

          t_Name_label->set_alignment(0.0);
          t_Type_label->set_alignment(0.0);
          t_dateMod_label->set_alignment(0.0);
          t_ArtistName_label->set_alignment(0.0);


          //Create Vertical Seperator Between Label & Label Data
          Gtk::VSeparator *t_VBreaker = new Gtk::VSeparator();
          m_label_data_seps.push_back(t_VBreaker);
          t_HBox_ld->pack_start(*t_VBreaker, false, false, 10);

          //Create & Embed Label Data Container
          Gtk::VBox *t_VBox_d = new Gtk::VBox;
          m_data_conts.push_back(t_VBox_d);
          t_HBox_ld->pack_start(*t_VBox_d, false, false, 0);
          t_VBox_d->set_spacing(4);

          //Label Data Creation
          Gtk::Label *t_Name_d = new Gtk::Label((*mat_iter)->name());
          Gtk::Label *t_Type_d = new Gtk::Label((*mat_iter)->type());
          Gtk::Label *t_dateMod_d = new Gtk::Label((*mat_iter)->dateStamp());
          Gtk::Label *t_ArtistName_d = new Gtk::Label((*mat_iter)->artistName());

          m_name_data_labels.push_back(t_Name_d);
          m_type_data_labels.push_back(t_Type_d);
          m_datemod_data_labels.push_back(t_dateMod_d);
          m_artistname_data_labels.push_back(t_ArtistName_d);

          t_VBox_d->pack_start(*t_Name_d, false, false, 0);
          t_VBox_d->pack_start(*t_Type_d, false, false, 0);
          t_VBox_d->pack_start(*t_dateMod_d, false, false, 0);
          t_VBox_d->pack_start(*t_ArtistName_d, false, false, 0);

          t_Name_d->set_alignment(0.0);
          t_Type_d->set_alignment(0.0);
          t_dateMod_d->set_alignment(0.0);
          t_ArtistName_d->set_alignment(0.0);

          //Create Seperator Between Labels & Artist Notes
          Gtk::HSeparator *t_DataHBreaker = new Gtk::HSeparator();
          m_data_notes_seps.push_back(t_DataHBreaker);
          t_VBox_dd->pack_start(*t_DataHBreaker, false, false, 0);

          //Create The Artist Notes ML Text Widget
          Glib::RefPtr<Gtk::TextBuffer> txtDisplay = Gtk::TextBuffer::create();
          txtDisplay->set_text((*mat_iter)->artistNotes());

          Gtk::TextView *t_NotesView = new Gtk::TextView(txtDisplay);
          t_NotesView->set_editable(false);	    
          m_artistnotes_mltext.push_back(t_NotesView);

          //Embed The ml Text Widget Into A Scroll Window
          Gtk::ScrolledWindow *t_NotesScrollWin = new Gtk::ScrolledWindow;
          m_artistnotes_scrollwin.push_back(t_NotesScrollWin);
          t_NotesScrollWin->add(*t_NotesView);
          t_VBox_dd->pack_start(*t_NotesScrollWin, true, true, 0);

          //Create Seperator Between Each Material Section
          Gtk::HSeparator *t_HBreaker = new Gtk::HSeparator();
          m_materials_seps.push_back(t_HBreaker);
          m_materials_cont.pack_start(*t_HBreaker, false, false, 0);

          //Increment Filename Tag For Unique Identifier
          ++fileName_int;

        }//for

      //Gtk Build / Show GUI Hint
      m_hpane->show_all(); 


     //  k3d::bool_t toggle_alpha = checkPviewBackground((*mat_iter));


//       k3d::log() << "toggle_alpha ok" << std::endl;

//       rEngineAlpha(toggle_alpha, (*mat_iter));

//       k3d::log() << "renginealpha ok" << std::endl;

      matobjAttachGeo();

      //Set Off Renderer In New Process 
      renderPreview();
    }
  else
    ; // Invalid HPanel Pointer 

}//buildPanel


void GroupContentPanel::renderPreview()
{
  //Invoke Generic Render Initialization
  renderInit();

 
  //Iterate Through Each Material In Group & Render

  std::list<MaterialObj*>::const_iterator mat_iter = m_materialgrp->materialBegin();

  for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
    {
      //Check If Selected Node Is A RenderMan Material
      if((*mat_iter)->isMaterial())
        {
//Set Alpha Render In Render Engine
          k3d::bool_t toggle_alpha = checkPviewBackground(*mat_iter);
          rEngineAlpha(toggle_alpha, *mat_iter);


          //Select Correct Geometry
          k3d::inode *selected_geo = (*mat_iter)->m_preview_geo;

          // k3d::property::set_internal_value(*m_engine, 
//                                             "visible_nodes", 
//                                             k3d::inode_collection_property
//                                             ::nodes_t(1, m_geometry));

          
          k3d::property::set_internal_value(*m_engine, 
                                            "visible_nodes", 
                                            k3d::inode_collection_property
                                            ::nodes_t(1, selected_geo));

          //If It Is, Assign To Current Geometry As A Surface Shader
          k3d::property
            ::set_internal_value(*get_material_sink(selected_geo, m_document_state->document().pipeline()), 
                                 "material", 
                                 const_cast<k3d::inode*>((*mat_iter)->docNode()));

          //Render The Preview Using Selected External Renderer
          m_engine
            ->render_camera_frame(*m_camera, 
                                  (const_cast<RenderedImage*>((*mat_iter)->pviewImg()))->imgFilePath(), false);

        }//if	 

      else
        {
          ; //Not A Renderman Material
        }

    }//for  

}//renderPreview



bool GroupContentPanel::updatePreviewImage()
{
  //Invoke A Gtk Image Update / Refresh For Each Preview Image
  std::list<MaterialObj*>::const_iterator mat_iter = m_materialgrp->materialBegin();

  for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
    {
      (const_cast<RenderedImage*>((*mat_iter)->pviewImg()))->queue_resize();
      (const_cast<RenderedImage*>((*mat_iter)->pviewImg()))->queue_draw();
    }
	    
  return true;
}



void GroupContentPanel::renderSinglePreview(k3d::inode *node)
{
  //Iterate Through All Of The Stored MaterialObj's.
   std::list<MaterialObj*>::const_iterator mat_iter = m_materialgrp->materialBegin();

   MaterialObj* matching_material = 0;

   for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
   {
     //Check If MaterialObj's Doc Node Equals Argument Node
     if((*mat_iter)->m_doc_node == node)
       {
         matching_material = (*mat_iter);
         break;
       }
   }//for

   //Start A Single Render Preview
   if(matching_material)
     {
       //Invoke Generic Render Initialization
       renderInit();


       k3d::property::set_internal_value(*m_engine, 
                                         "visible_nodes", 
                                         k3d::inode_collection_property
                                         ::nodes_t(1, m_geometry));


       //Check If Selected Node Is A RenderMan Material
       if(matching_material->isMaterial())
         {
           //If It Is, Assign To Current Geometry As A Surface Shader
           k3d::property
             ::set_internal_value(*get_material_sink(m_geometry, m_document_state->document().pipeline()), 
                                  "material", 
                                  const_cast<k3d::inode*>(matching_material->docNode()));


           //Get Path Of Target Render Image
           k3d::filesystem::path pimg_path 
             =(const_cast<RenderedImage*>((*mat_iter)->pviewImg()))->imgFilePath();

           //Render The Preview Using Selected External Renderer
           m_engine->render_camera_frame(*m_camera, 
                                         pimg_path,
                                         false);

    }//if	 

  else
    {
      ; //Not A Renderman Material
    }



     }//if
  
}


bool GroupContentPanel::findMaterial(const k3d::inode *node)
{
  //Iterate Through All Of The Stored MaterialObj's.
   std::list<MaterialObj*>::const_iterator mat_iter = m_materialgrp->materialBegin();

   for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
     {
       if((*mat_iter)->docNode() == node)
         {
           return true;
         }
     }

   return false;

}



bool GroupContentPanel::findMaterial(const MaterialObj *mat)
{
  //Iterate Through All Of The Stored MaterialObj's.
  std::list<MaterialObj*>::const_iterator mat_iter = m_materialgrp->materialBegin();

  for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
     {
       if(*mat_iter == mat)
         {
           return true;
         }
     }
  
  return false;
}




void GroupContentPanel::matobjAttachGeo()
{
  //Iterate Through All Of The Stored MaterialObj's.
  std::list<MaterialObj*>::const_iterator mat_iter = m_materialgrp->materialBegin();

  for(; mat_iter != m_materialgrp->materialEnd(); ++mat_iter)
    {
      //Get The Doc Node From MaterialObj Ptr
      k3d::inode *mat_node = (*mat_iter)->m_doc_node;

      //Geometry Flags For Combo Update
      bool sphereUsed = false;
      bool cubeUsed = false;
      bool torusUsed = false;

      //Search m_doc_node Material For Attached Geometry
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(mat_node))
        {
          k3d::string_t value = metadata->get_metadata()[MaterialObj::attached_geo_nametag_mt];

          k3d::inode *attached_geo = 0; //Geometry Is Already Availible
          PreviewObj *attached_new_geo = 0; //Geometery Is New & Needs To Be Created

          k3d::string_t meta_attachedgeo = "";

          k3d::log() << "VALUE: " << value << std::endl;
          
          //Check If Sphere Attached
          if(value == PreviewObj::sphere_md)
            {
              //Check If Doesnt Exist In Document
              if(!checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::sphere_md, &attached_geo, m_document_state))
                {
                  //Create The Default Sphere For Preview
                  attached_new_geo = new PreviewSphere("Sphere", m_document_state);
                  attached_new_geo->init(PreviewObj::sphere_node_name, PreviewObj::sphere_md, m_engine);
                }

              meta_attachedgeo = PreviewObj::sphere_md;
              sphereUsed = true;
            }

          //Check If Cube Attached
          else if(value == PreviewObj::cube_md)
            {
              //Check If Doesnt Exist In Document
              if(!checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::cube_md, &attached_geo, m_document_state))
                {
                  //Create The Default Cube For Preview
                  attached_new_geo = new PreviewCube("Cube", m_document_state);
                  attached_new_geo->init(PreviewObj::cube_node_name, PreviewObj::cube_md, m_engine);
                }

              meta_attachedgeo = PreviewObj::cube_md;
              cubeUsed = true;

            }

          //Check If Torus Attached
          else if(value == PreviewObj::torus_md)
            {
              //Check If Doesnt Exist In Document
              if(!checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::torus_md, &attached_geo, m_document_state))
                {

                  k3d::log() << "TORUS NEW BUILT!" << std::endl;
                  //Create The Default Torus For Preview
                  attached_new_geo = new PreviewTorus("torus", m_document_state);
                  attached_new_geo->init(PreviewObj::torus_node_name, PreviewObj::torus_md, m_engine);
                }

              meta_attachedgeo = PreviewObj::torus_md;
              torusUsed = true;
            }

          else
            {
              //No Meta Data. Possibly Create & Attach Default Sphere
              //k3d::inode *existing_sphere = 0;
              if(!checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::sphere_md, &attached_geo, m_document_state))
                {
                  attached_new_geo =  new PreviewSphere("Sphere", m_document_state);
                  attached_new_geo->init(PreviewObj::sphere_node_name, PreviewObj::sphere_md, m_engine);

                  (*mat_iter)->setPreviewGeo(attached_new_geo->m_doc_node, PreviewObj::sphere_md);

                  k3d::log() << "no meta, sphere node created: " << attached_new_geo->m_doc_node << std::endl;

                }
              else
                {
                  //Default Sphere Exists In Document
                  if(attached_geo)
                    {
                      (*mat_iter)->setPreviewGeo(attached_geo, PreviewObj::sphere_md);
                      k3d::log() << "no meta, sphere node Not created: " << attached_geo << std::endl;
                    }

                }


              meta_attachedgeo = PreviewObj::sphere_md;
              sphereUsed = true;


            }


          //Attach The Geometry To The MaterialObj**********************************

          if(attached_geo)
            {
              (*mat_iter)->setPreviewGeo(attached_geo, meta_attachedgeo);
              k3d::log() << "attached geo: " << (*mat_iter)->m_preview_geo << std::endl;
            }

          else if(attached_new_geo)
            {
              (*mat_iter)->setPreviewGeo(attached_new_geo->m_doc_node, meta_attachedgeo);
              m_used_geometry.push_back(attached_new_geo);
              k3d::log() << "attached new geo: " << (*mat_iter)->m_preview_geo << std::endl;
            }


          //************************************************************************

              k3d::log() << "OI HERE BABY!" << std::endl;

        }//if
      

     


    }//for


}


}//namespace mechanics

}//namespace material_manager

}//namespace ngui

}//namespace module
