#include "ContentPanel.h"
#include <k3dsdk/inode_collection_sink.h>

namespace module
{
namespace ngui
{
namespace material_manager
{
namespace mechanics
{

// bool ContentPanel::checkDocForMeta(const k3d::string_t meta_tag, 
//                                    const k3d::string_t meta_data, 
//                                    k3d::inode **node_ptr)
// {
//   //Iterate Through Document. If A Node Has Meta Data & Matches, Pass To node_ptr
//   k3d::inode_collection::nodes_t::const_iterator node 
//     = m_document_state->document().nodes().collection().begin();

//   for(; node != m_document_state->document().nodes().collection().end(); ++node)
//     {
//       if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(*node))
//         {
//           k3d::string_t value = metadata->get_metadata()[meta_tag];
          
//           if(value == meta_data)
//             {
//               //There Is A Match!
//               *node_ptr = *node;
//               return true;
//             }
//         }//if
//     }//for

//   //No Match Found
//   node_ptr = 0;
//   return false;

// }//checkDocForMeta



void ContentPanel::createPreviewNodes()
{
  //Flags For Each Node
  bool hasAqsis_renderer = 	false;
  bool hasCamera = 				false;
  bool hasGeoSphere = 		  	false;
  bool hasGeoCube = 		 	 	false;
  bool hasGeoTorus = 		  	false;
  bool hasLight = 				false;
  bool hasLightFill = 			false;
  bool hasLightBack = 			false;
  bool hasLight_shader = 		false;
  bool hasLightFill_shader = 	false;
  bool hasRenderman_engine = 	false;


  //Meta Data Strings
  k3d::string_t aqsis_render_meta 	= "p_aqsis_renderer";
  k3d::string_t camera_meta 			= "p_camera";

  // k3d::string_t sphere_geo_meta 		= "p_sphere_geo";
//   k3d::string_t cube_geo_meta 		= "p_cube_geo";
//   k3d::string_t torus_geo_meta 		= "p_torus_geo";

  k3d::string_t key_light_meta 		= "p_light";
  k3d::string_t fill_light_meta 		= "p_fill_light";
  k3d::string_t back_light_meta 		= "p_back_light";

  k3d::string_t key_bck_lshade_meta = "p_light_shader";
  k3d::string_t fill_lshade_meta 	= "p_fill_light_shader";
  
  k3d::string_t rman_engine_meta   	= "p_rman_engine";

  k3d::string_t nametag_metatag 		= "PreviewCore::nametag";

  //Pointer To Aqsis Engine For RMAN Engine Node
  k3d::ri::irender_engine* aqsis = 0;
  k3d::string_t pointLightPath = "shaders/light/k3d_pointlight.sl";

  //Check Nodes MetaData To See If These Nodes Exist.
  //If They Do Dont Exist Then Create New Nodes

  k3d::inode *node_ptr = 0;
  
  //Check For Aqsis Engine
  if(checkDocForMeta(nametag_metatag, aqsis_render_meta, &node_ptr, m_document_state))
    {
      hasAqsis_renderer = true;
      if(node_ptr)
        aqsis = dynamic_cast<k3d::ri::irender_engine*>(node_ptr);

    }

  //Check For Camera
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, camera_meta, &node_ptr, m_document_state))
    {
       hasCamera = true;
       if(node_ptr)
         m_camera = dynamic_cast<camera_t*>(node_ptr);
    }

                                     
  //Check For Geometry**********************************************************

  //Check For Sphere Geometry
  node_ptr = 0;
  if(checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::sphere_md, &node_ptr, m_document_state))
    {
      hasGeoSphere = true;
      if(node_ptr)
        m_geometry = dynamic_cast<geo_t*>(node_ptr);
    }

  //Check For Cube Geometry
  node_ptr = 0;
  if(checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::cube_md, &node_ptr, m_document_state))
    {
      hasGeoCube = true;
      if(node_ptr)
        m_geometry = dynamic_cast<geo_t*>(node_ptr);
    }

  //Check For Torus Geometry
  node_ptr = 0;
  if(checkDocForMeta(PreviewObj::pview_geo_nametag_mt, PreviewObj::torus_md, &node_ptr, m_document_state))
    {
      hasGeoTorus = true;
      if(node_ptr)
        m_geometry = dynamic_cast<geo_t*>(node_ptr);
    }

  //****************************************************************************


  //Check For Main Key Light
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, key_light_meta, &node_ptr, m_document_state))
    {
      hasLight = true;
      if(node_ptr)
        m_main_light = dynamic_cast<light_t*>(node_ptr);
    }


  //Check For Fill Light
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, fill_light_meta, &node_ptr, m_document_state))
    {
      hasLightFill = true;
      if(node_ptr)
        m_fill_light = dynamic_cast<light_t*>(node_ptr);
    }


  //Check For Back Light
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, back_light_meta, &node_ptr, m_document_state))
    {
      hasLightBack = true;
      if(node_ptr)
        m_back_light = dynamic_cast<light_t*>(node_ptr);
    }


  //Check For Key + Back Light Shader
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, key_bck_lshade_meta, &node_ptr, m_document_state))
    {
      hasLight_shader = true;
      if(node_ptr)
        m_light_shader = dynamic_cast<k3d::inode*>(node_ptr);	
    }


  //Check For Fill Light Shader
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, fill_lshade_meta, &node_ptr, m_document_state))
    {
      hasLightFill_shader = true;
      if(node_ptr)
        m_fill_light_shader = dynamic_cast<k3d::inode*>(node_ptr);
    }


  //Check For Render Engine
  node_ptr = 0;
  if(checkDocForMeta(nametag_metatag, rman_engine_meta, &node_ptr, m_document_state))
    {
      hasRenderman_engine = true;
      if(node_ptr)
        m_engine = dynamic_cast<rManEngine_t*>(node_ptr);
    }




  //Setup Light Shader Preview Node**********

  if(!hasLight_shader)
    {
      m_light_shader 
        = dynamic_cast<k3d::inode*>(k3d::plugin::create("RenderManLightShader", 
                                                        m_document_state->document(), 
                                                        "Preview Core::Light Shader"));

      k3d::property::set_internal_value(*m_light_shader, 
                                        "shader_path", k3d::share_path() /
                                        k3d::filesystem::generic_path(pointLightPath));


      k3d::property::set_internal_value(*m_light_shader, 
                                        "intensity", k3d::double_t(1200));
	      
      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_light_shader))
        metadata->set_metadata_value(nametag_metatag, key_bck_lshade_meta);
   
    }//if


  if(!hasLightFill_shader)
    {
      m_fill_light_shader
        = dynamic_cast<k3d::inode*>(k3d::plugin::create("RenderManLightShader", 
                                                        m_document_state->document(), 
                                                        "Preview Core::Fill Light Shader"));

      k3d::property::set_internal_value(*m_fill_light_shader, 
                                        "shader_path", k3d::share_path() /
                                        k3d::filesystem::generic_path(pointLightPath));

      k3d::property::set_internal_value(*m_fill_light_shader, 
                                        "intensity", k3d::double_t(650));
	      
      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_fill_light_shader))
        metadata->set_metadata_value(nametag_metatag, fill_lshade_meta);
   
    }//if

  //Setup The Light**********

  if(!hasLight)
    {
      m_main_light 
        = dynamic_cast<light_t*>(k3d::plugin::create("RenderManLight", 
                                                     m_document_state->document(), 
                                                     "Preview Core::Light"));

      k3d::property::set_internal_value(*m_main_light, 
                                        "shader", m_light_shader);


      k3d::inode* light_transformation 
        = k3d::set_matrix(*m_main_light, 
                          k3d::translate3(-20, 20, 20));
      

      k3d::property::set_internal_value(*m_main_light, 
                                        "viewport_visible", false);


      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_main_light))
        metadata->set_metadata_value(nametag_metatag, key_light_meta);

    }//if


  if(!hasLightFill)
    {
      m_fill_light 
        = dynamic_cast<light_t*>(k3d::plugin::create("RenderManLight", 
                                                     m_document_state->document(), 
                                                     "Preview Core::Fill Light"));

      k3d::property::set_internal_value(*m_fill_light, 
                                        "shader", m_fill_light_shader);


      k3d::inode* light_transformation 
        = k3d::set_matrix(*m_fill_light, 
                          k3d::translate3(20, 28, -18));


      k3d::property::set_internal_value(*m_fill_light, 
                                        "viewport_visible", false);


      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_fill_light))
        metadata->set_metadata_value(nametag_metatag, fill_light_meta);

    }//if


  if(!hasLightBack)
    {
      m_back_light 
        = dynamic_cast<light_t*>(k3d::plugin::create("RenderManLight", 
                                                     m_document_state->document(), 
                                                     "Preview Core::Back Light"));

      k3d::property::set_internal_value(*m_back_light, 
                                        "shader", m_light_shader);


      k3d::inode* light_transformation 
        = k3d::set_matrix(*m_back_light, 
                          k3d::translate3(0, -38, 18));


      k3d::property::set_internal_value(*m_back_light, 
                                        "viewport_visible", false);

      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_back_light))
        metadata->set_metadata_value(nametag_metatag, back_light_meta);

    }//if


  //Setup The Camera**********
	  
  if(!hasCamera)
    {
      m_camera 
        = dynamic_cast<camera_t*>(k3d::plugin::create("Camera", 
                                                      m_document_state->document(), 
                                                      "Preview Core::Camera"));

      //Orientate The Camera In The World
      const k3d::point3 origin = k3d::point3(0, 0, 0);
      const k3d::vector3 world_up = k3d::vector3(0, 0, 1);
      const k3d::point3 position = k3d::point3(0, 13, 0);
      const k3d::vector3 look_vector = origin - position;
      const k3d::vector3 right_vector = look_vector ^ world_up;
      const k3d::vector3 up_vector = right_vector ^ look_vector;

      k3d::inode* const camera_transformation 
        = k3d::set_matrix(*m_camera, 
                          k3d::view_matrix(look_vector, up_vector, position));
       
      camera_transformation->set_name("Camera Transformation");

      k3d::property::set_internal_value(*m_camera, 
                                        "world_target", k3d::point3(0, 0, 0));

      k3d::property::set_internal_value(*m_camera, 
                                        "viewport_visible", false);

      k3d::property::set_internal_value(*m_camera, 
                                        "aspect_ratio", k3d::string_t("Square"));

      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_camera))
        metadata->set_metadata_value(nametag_metatag, camera_meta);

    }//if

  //****************************************************************************

  //Setup The Aqsis Renderman Engine**********

  if(!hasAqsis_renderer)
    {
			// Try the bundled engine first, for win32 installed versions
			aqsis 
  	        = k3d::plugin::create<k3d::ri::irender_engine>("BundledAqsisRenderManEngine", 
  	                                                       m_document_state->document(), 
  	                                                       "Preview Core::Aqsis Renderer");
  	  
  	  // bundled engine is not installed, remove it
  	  if(aqsis && !aqsis->installed())
  	  {
  	  	k3d::delete_nodes(m_document_state->document(), k3d::nodes_t(1, dynamic_cast<k3d::inode*>(aqsis)));
  	  	aqsis = 0;
  	  }
  	  
  	  if(!aqsis)
  	  {
				aqsis 
					= k3d::plugin::create<k3d::ri::irender_engine>("AqsisRenderManEngine", 
                                                              m_document_state->document(), 
                                                              "Preview Core::Aqsis Renderer");
  	  }

      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(aqsis))
        metadata->set_metadata_value(nametag_metatag, aqsis_render_meta);

    }//if


  //Setup The Renderman Engine

  if(!hasRenderman_engine)
    {
      m_engine 
        = dynamic_cast<rManEngine_t*>(k3d::plugin::create("RenderManEngine", 
                                                         m_document_state->document(), 
                                                          "Preview Core::RenderManEngine"));

      k3d::property::set_internal_value(*m_engine, 
                                        "enabled_lights", 
                                        k3d::inode_collection_property
                                        ::nodes_t(1, m_main_light));

      k3d::property::set_internal_value(*m_engine, 
                                        "visible_nodes", 
                                        k3d::inode_collection_property
                                        ::nodes_t(1, m_geometry));


      //Assign Aqsis To The Chosen RenderEngine Slot

      k3d::property::set_internal_value(*m_engine, 
                                        "render_engine", 
                                        dynamic_cast<k3d::inode*>(aqsis));

      //Setup Preview Render Dimensions

      k3d::property::set_internal_value(*m_engine, 
                                        "pixel_width", 
                                        static_cast<k3d::int32_t>(m_pview_size));


      k3d::property::set_internal_value(*m_engine, 
                                        "pixel_height", 
                                        static_cast<k3d::int32_t>(m_pview_size));

      k3d::double_t aspectRatio = 1.0;

      k3d::property::set_internal_value(*m_engine, 
                                        "pixel_aspect_ratio", aspectRatio);


      //Set Render Quality Attributes: Optimized For Preview Render
      k3d::double_t pixel_xy_samples = 1.0;

      k3d::property::set_internal_value(*m_engine, 
                                        "pixel_xsamples", pixel_xy_samples);

      k3d::property::set_internal_value(*m_engine, 
                                        "pixel_ysamples", pixel_xy_samples);


      k3d::double_t shading_rate = 8.0;

      k3d::property::set_internal_value(*m_engine, 
                                        "shading_rate", shading_rate);

      //Create Meta Data
      if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(m_engine))
        metadata->set_metadata_value(nametag_metatag, rman_engine_meta);
      

    }//if
  
  //Setup The Geometry**********************************************************

  if(!hasGeoSphere)
    {
      //Create The Default Sphere Geometric Object
      PreviewSphere *default_sphereObj = new PreviewSphere("Sphere", m_document_state);
      default_sphereObj->init(PreviewObj::sphere_node_name, PreviewObj::sphere_md, m_engine);
      m_used_geometry.push_back(default_sphereObj);

      //Set Current Geometry To The Sphere
      m_geometry = default_sphereObj->m_doc_node;

    }//if

 //  if(!hasGeoCube)
//     {
//       //Create The Default Cube Geometric Object
//       PreviewCube *default_cubeObj = new PreviewCube("Cube", m_document_state);
//       default_cubeObj->init(PreviewObj::cube_node_name, PreviewObj::cube_md);
//       m_used_geometry.push_back(default_cubeObj);

//       //Set Current Geometry To The Sphere
//       m_geometry = default_cubeObj->m_doc_node;

//     }//if

//   if(!hasGeoTorus)
//     {
//       //Create The Default Torus Geometric Object
//       PreviewTorus *default_torusObj = new PreviewTorus("Torus", m_document_state);
//       default_torusObj->init(PreviewObj::torus_node_name, PreviewObj::torus_md);
//       m_used_geometry.push_back(default_torusObj);

//       //Set Current Geometry To The Sphere
//       m_geometry = default_torusObj->m_doc_node;

//     }//if

}//createPreviewNodes



void ContentPanel::renderInit()
{
  //Re-init The Preview Render Dimensions
  k3d::property::set_internal_value(*m_engine, 
                                    "pixel_width", 
                                    static_cast<k3d::int32_t>(m_pview_size));


  k3d::property::set_internal_value(*m_engine, 
                                    "pixel_height", 
                                    static_cast<k3d::int32_t>(m_pview_size));


  //Ensure Current Preview Engine Has Selected Nodes Only Visible
  k3d::inode_collection::nodes_t::const_iterator node 
    = m_document_state->document().nodes().collection().begin();

  for(; node != m_document_state->document().nodes().collection().end(); ++node)
    {
      if((*node)->factory().implements(typeid(k3d::ri::ilight)))
        {
          //Disable Node Regardless In RMANEngine::lights and nodes
          k3d::property::set_internal_value(*m_engine, 
                                            "enabled_lights", 
                                            k3d::inode_collection_property
                                            ::nodes_t(0, (*node)));
        }//if
      else if((*node)->factory().implements(typeid(k3d::imatrix_sink)))
        {
          k3d::property::set_internal_value(*m_engine, 
                                            "visible_nodes", 
                                            k3d::inode_collection_property
                                            ::nodes_t(0, (*node)));
        }//else if
	    
    }//for

  //Vector List Of Lights To Be Enabled In Chosen Render Engine
  std::vector<k3d::inode*>lightsEnabled;
  lightsEnabled.push_back(m_main_light);
  lightsEnabled.push_back(m_fill_light);
  lightsEnabled.push_back(m_back_light);

  //Simply Enable Now Only USed Lights & Geo
  k3d::property::set_internal_value(*m_engine, 
                                    "enabled_lights", lightsEnabled);
 


  // k3d::property::set_internal_value(*m_engine, 
//                                     "visible_nodes", 
//                                     k3d::inode_collection_property
//                                     ::nodes_t(1, m_geometry));

}//renderInit



void ContentPanel::rEngineAlpha(k3d::bool_t toggle, MaterialObj *mat)
{
  if(m_engine)
    {
      //Turn The Render Alpha Property On Or Off Depending On Switch
      k3d::property::set_internal_value(*m_engine, 
                                        "render_alpha", 
                                        toggle);

    }//if


  //Get The Doc Node From MaterialObj Ptr
  k3d::inode *mat_node = mat->m_doc_node;

  if(mat_node)
    {
      //Set The MetaData For Background
      if(toggle)
        {
          //Set MetaData To Show Background
          if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(mat_node))
            metadata->set_metadata_value(MaterialObj::show_bg_nametag_mt, MaterialObj::do_show_bg);
        }
      else
        {
          //Set MetaData To NOT Show Background
          if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(mat_node))
            metadata->set_metadata_value(MaterialObj::show_bg_nametag_mt, MaterialObj::do_not_show_bg);
        }
    }


}//rEngineAlpha




bool ContentPanel::checkPviewBackground(MaterialObj *mat)
{
  //Get The Doc Node From MaterialObj Ptr
  k3d::inode *mat_node = mat->m_doc_node;

  //Search m_doc_node Material For Attached Geometry
  if(k3d::imetadata* const metadata = dynamic_cast<k3d::imetadata*>(mat_node))
    {
      k3d::string_t value = metadata->get_metadata()[MaterialObj::show_bg_nametag_mt];

      if(value == MaterialObj::do_show_bg)
        {
          //Background Needs To Be Shown
          return true;
        }
      else if(value == MaterialObj::do_not_show_bg)
        {
          //Background Does Not Need To Be Shown
          return false;
        }
      else
        {
          //DEFAULT: Background Does Not Need To Be Shown
          return false;
        }



    }//if


}//checkPviewBackground



}//namespace mechanics

}//namespace material_manager

}//namespace ngui

}//namespace module
