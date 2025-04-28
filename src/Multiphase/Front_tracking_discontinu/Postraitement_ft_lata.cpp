/****************************************************************************
* Copyright (c) 2015 - 2016, CEA
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

#include <Postraitement_ft_lata.h>
#include <Probleme_FT_Disc_gen.h>
#include <Localization.h>
#include <Transport_Marqueur_FT.h>
#include <Motcle.h>
#include <Domaine.h>
#include <EcrFicPartage.h>
#include <EcrFicPartageBin.h>
#include <Fichier_Lata.h>
#include <TRUSTTab.h>
#include <communications.h>
#include <SFichier.h>
#include <Param.h>
#include <Format_Post_Lata.h>

Implemente_instanciable(Postraitement_ft_lata,"Postraitement_ft_lata",Postraitement);

Entree& Postraitement_ft_lata::readOn(Entree& is)
{
  // Verification du type du probleme
  const Nom& type_pb =  mon_probleme->que_suis_je();
  if ( (!sub_type(Probleme_FT_Disc_gen, mon_probleme.valeur()))
       && (type_pb!="Pb_Thermohydraulique_Especes_QC")
       && (type_pb!="Pb_Thermohydraulique_Especes_Turbulent_QC") )
    {

      Cerr << " Reading Postraitement_ft_lata\n";
      Cerr << " postraitement_ft_lata is not accepted for a problem of type "<<type_pb << finl;
      Cerr << " The recognized problems are :" << finl;
      Cerr << " Probleme_FT_Disc_gen " << finl;
      Cerr << " Pb_Thermohydraulique_Especes_QC " << finl;
      Cerr << " Pb_Thermohydraulique_Especes_Turbulent_QC " << finl;
      Process::exit();
    }

  Postraitement::readOn(is);
  if (!champs_demande_)
    {
      Cerr << "*********************************************************************" << finl;
      Cerr << "Warning: in Postraitement_ft_lata block, you specified interfaces to post-process" << finl;
      Cerr << "without specifying fields. Interfaces will not be post-processed unless you post-process a field also." << finl;
      Cerr << "Contact TRUST/TrioCFD support team or look for examples in TrioCFD databases" << finl;
      Cerr << "*********************************************************************" << finl;
    }

  if (!sub_type(Format_Post_Lata, format_post.valeur()))
    {
      Cerr << "****************************************************************************" << finl;
      Cerr << "   WARNING WARNING " << finl;
      Cerr << "      You are processing a FT problem, but you did not use the 'LATA' format (directive 'format lata')." << finl;
      Cerr << "      The interfaces will **not** be post-processed and will be ignored." << finl;
      Cerr << "****************************************************************************" << finl;
    }

  return is;
}

Sortie& Postraitement_ft_lata::printOn(Sortie& os) const
{
  return os;
}

void Postraitement_ft_lata::set_param(Param& param)
{
  Postraitement::set_param(param);
  param.ajouter_non_std("interfaces",(this));
}

int Postraitement_ft_lata::lire_motcle_non_standard(const Motcle& mot, Entree& is)
{
  int ret_val = Postraitement::lire_motcle_non_standard(mot, is);
  if (ret_val != -1) // all good we've hit standard postprocessing keywords
    return ret_val;

  Motcle motlu;
  if (mot=="interfaces")
    {
      is >> motlu;
      if (refequation_interfaces.non_nul())
        {
          Cerr<<" Only one transport interface equation name can be specified "<<finl;
          Cerr<<" for a Postraitement_ft_lata post-process."<<finl;
          Cerr<<" The "<<Motcle(refequation_interfaces->le_nom())<<" has already been read for the post-process "<<(*this).le_nom()<<finl;
          Cerr<<" Please, create a new Postraitement_ft_lata post-process"<<finl;
          Cerr<<" for the "<<motlu<<" transport interface equation."<<finl;
          Process::exit();
        }

      if (Process::je_suis_maitre())
        Cerr << "Post-processing for the interface of transport equation : " << motlu << finl;

      if (sub_type(Probleme_FT_Disc_gen, mon_probleme.valeur()))
        {
          const Probleme_FT_Disc_gen& pb =
            ref_cast(Probleme_FT_Disc_gen, mon_probleme.valeur());
          refequation_interfaces = pb.equation_interfaces(motlu);
        }
      else
        for (int i=0; i<mon_probleme->nombre_d_equations(); i++)
          {
            const Nom& nom_eq =  mon_probleme->equation(i).le_nom();
            if (sub_type(Transport_Interfaces_FT_Disc,mon_probleme->equation(i))
                && (motlu==Motcle(nom_eq)))
              {
                refequation_interfaces=ref_cast(Transport_Interfaces_FT_Disc,mon_probleme->equation(i));
              }
          }

      if (!refequation_interfaces.non_nul())
        {
          Cerr<<" No interface equation name  "<<motlu<<" has been found. "<<finl;
          Process::exit();
        }
      is >> motlu;
      if (motlu == "no_virtuals")
        {
          if (Process::nproc() > 1)
            no_virtuals_ = true;
          is >> motlu;
        }
      if (motlu != "{")
        {
          Cerr << " Postraitement_ft_lata::lire_champ\n";
          Cerr << " { was expected after the keyword interfaces\n";
          Cerr << " It has been found " << motlu << finl;
          Process::exit();
        }

      lire_champ_interface(is);

      // This is put here to make sure we have read all keywords anyway:
      if( Motcle(format).debute_par("LATA")==0)
        {
          Cerr << "**** Output format is not LATA, interfaces will be silently ignored." << finl;
          // Nullify reference, this will skip interfaces in the rest of the process
          refequation_interfaces.reset();
        }

      return 1;
    }
  else
    return -11;
}

/*! @brief lecture de la liste de champs aux interfaces a postraiter
 */
void Postraitement_ft_lata::lire_champ_interface(Entree& is)
{
  Motcle field_name, localization_name;
  const Transport_Interfaces_FT_Disc& eq_interfaces = refequation_interfaces.valeur();

  while (1)
    {
	  // read the field name
      is >> field_name;
      if (field_name == "}")  break;
	  
	  // read the localization of the field
      is >> localization_name;
      Localization localization = Localization::Unknown;
      if        (localization_name == "som") {
        localization = Localization::Vertex;
      } else if (localization_name == "elem") {
        localization = Localization::Element;
      } else if (localization_name == "connected_component") {
        localization = Localization::ConnectedComponent;
      } else
        {
          Cerr << "Error for Postraitement_ft_lata::lire_champ_interface :\n";
          Cerr << " Keywords 'som', 'elem' or 'particle' were expected after the field name '" << field_name << "' (got '" << localization_name << "')." << finl;
          Process::exit();
        }
	  assert(localization != Localization::Unknown);

      // check if we can retrieve this field as a DoubleTab or a IntTab
      if (!(eq_interfaces.is_field_available<double>(field_name, localization)) && !(eq_interfaces.is_field_available<int>(field_name, localization)))
        {
          Cerr << "Error for Postraitement_ft_lata::lire_champ_interface :\n";
          Cerr << " The field '" << field_name << "' is not understood for the " << (eq_interfaces.que_suis_je()=="Transport_Marqueur_FT"?"particules":"interfaces") << " or not authorized at localisation '" << to_string(localization) << "'." << finl;
          //eq_interfaces.get_field(demande_description, loc, (DoubleTab*) 0);
          //eq_interfaces.get_field(demande_description, loc, (IntTab*) 0);
          Process::exit();
        }
	
	  // we don't check if a field is added twice, it would give the same result anyway
	  fields_to_postprocess[field_name.getString()].insert(localization);
    }
}

/*! @brief Build a reduced version of the facettes array, excluding virtual ones
 * Also update the internal renumbering array for later usage when writing out field values.
 */
int Postraitement_ft_lata::filter_out_virtual_fa7(IntTab& new_fa7)
{
  const Maillage_FT_Disc& mesh = refequation_interfaces->maillage_interface_pour_post();
  const IntTab& fa7 = mesh.facettes();
  int nl=fa7.dimension(0), nc=fa7.dimension(1);
  new_fa7.resize(0, nc);
  int sz = 0;

  // Reset renumbering
  renum_.clear();

  for(int i = 0; i < nl; i++)
    {
      if (!mesh.facette_virtuelle(i))
        {
          sz++;
          new_fa7.resize(sz, nc);
          renum_.push_back(i);
          for(int j = 0; j < nc; j++)
            new_fa7(sz-1, j) = fa7(i, j);
        }
    }
  return sz;
}

void Postraitement_ft_lata::filter_out_array(const DoubleTab& dtab, DoubleTab& new_dtab) const
{
  int nl = (int)renum_.size(), nc = dtab.dimension(1);
  new_dtab.resize(nl, nc);

  for(int i = 0; i < nl; i++)
    for(int j = 0; j < nc; j++)
      new_dtab(i, j) = dtab(renum_[i], j);
}

/*! @brief Override. Add the interfaces to the LATA output
 *
 */
int Postraitement_ft_lata::write_extra_mesh()
{
  if (refequation_interfaces.non_nul())
    {
      ecrire_maillage_ft_disc();
      return 1;
    }
  return -1;
}

/*! @brief Write the Maillage_FT_Disc object into a LATA file in V2 format.
 *
 */
int Postraitement_ft_lata::ecrire_maillage_ft_disc()
{
  // Determining mesh type according to equation type:
  if (sub_type(Transport_Marqueur_FT,refequation_interfaces.valeur()))
    id_domaine_ = "PARTICULES";
  else if (sub_type(Transport_Interfaces_FT_Disc,refequation_interfaces.valeur()))
    id_domaine_ = "INTERFACES";
  else
    {
      Cerr<<"Type "<<refequation_interfaces->que_suis_je()<<" not recognized"<<finl;
      Process::exit();
    }
  const Maillage_FT_Disc& mesh = refequation_interfaces->maillage_interface_pour_post();
  const DoubleTab& sommets = mesh.sommets();
  const IntTab& fa7 = mesh.facettes();
  int dim = mesh.sommets().dimension(1);
  Motcle type_elem = dim == 2 ? "Segment" : "Triangle";

  Format_Post_Lata& fpl = ref_cast(Format_Post_Lata, format_post.valeur());  // this is check above
  if (no_virtuals_)
    {
      IntTab real_fa7;
      filter_out_virtual_fa7(real_fa7);
      fpl.ecrire_domaine_low_level(id_domaine_, sommets, real_fa7, type_elem);
    }
  else
    fpl.ecrire_domaine_low_level(id_domaine_, sommets, fa7, type_elem);

  // if we write INTERFACES, we also write the center of gravity of every connected component
  if (sub_type(Transport_Interfaces_FT_Disc, refequation_interfaces.valeur()))
    {
	  // there is no element, vertices just represent the particles position
	  // but we still need to have a 2D tabular for elements with at least one component 
	  // otherwise the write tabulars functions crashes
	  IntTab no_elements(0, 1);
	  const DoubleTab& particles_positions = refequation_interfaces->get_particles_position();
	  fpl.ecrire_domaine_low_level("CONNECTED_COMPONENT", particles_positions, no_elements, type_elem);
	}

  return 1;
}

void Postraitement_ft_lata::postprocess_field_values()
{
  // The standard fields
  Postraitement::postprocess_field_values();

  // Now specific FT fields:
  if(!refequation_interfaces.non_nul()) return;

  const Transport_Interfaces_FT_Disc& eq_interfaces = refequation_interfaces.valeur();

  // iterator in fields to postprocess
  for (const auto& [field_name, localizations]: fields_to_postprocess)
    {
	  for (const Localization& localization: localizations)
	    {
		  Cerr << "WRITE FIELD: " << field_name << " at " << to_string(localization) << finl;

          DoubleTab dtab;

		  // try to get the tabular of doubles
          if (eq_interfaces.is_field_available<double>(field_name, localization))
            {
			  eq_interfaces.get_field<double>(field_name, localization, dtab);
            }
		  // if previous attempt failed, then it's a field of integers
          else if (eq_interfaces.is_field_available<int>(field_name, localization))
            {
              IntTab itab;
			  eq_interfaces.get_field<int>(field_name, localization, itab);

			  // copy the content of the integer field into the double tabular
              const int n = itab.dimension(0);
              const int m = itab.nb_dim() == 1 ? 1 : itab.dimension(1);
              dtab.resize(n, m);
              // copie du champ dans dtab
              if (itab.nb_dim() == 1)
                for (int j = 0; j < n; j++)
                  dtab(j,0) = static_cast<double>(itab(j));
              else
                for (int j = 0; j < n; j++)
                  for (int k = 0; k < m; k++)
                    dtab(j,k) = static_cast<double>(itab(j,k));
            }
          else
            {
			  // unavailable fields should have been check during the parsing of the
			  // postraitement block, but just in case
              Cerr << "Error for Postraitement_ft_lata::ecrire_maillage" << finl;
              Cerr << "Unknown field : " << field_name << finl;
              Process::exit();
            }

	      // check dimension
		  if      (localization == Localization::Element)
		      assert(dtab.dimension(0) == eq_interfaces.maillage_interface_pour_post().facettes().dimension(0));
	      else if (localization == Localization::Vertex)
		      assert(dtab.dimension(0) == eq_interfaces.maillage_interface_pour_post().sommets().dimension(0));
	      else if (localization == Localization::ConnectedComponent)
		      assert(dtab.dimension(0) == eq_interfaces.get_particles_position().dimension(0));
	      else
		      Process::exit("Invalid localization.");

          // write the field
		  if (localization == Localization::ConnectedComponent)
		    write_field("CONNECTED_COMPONENT", field_name, localization, dtab);
		  else
		    write_field(id_domaine_, field_name, localization, dtab);
		}
	}
}

void Postraitement_ft_lata::write_field(const Nom& domain_name, const Nom& field_name, const Localization& localization, const DoubleTab& values)
  {

    Cerr << "WRITING FIELD: " << field_name << " of " << domain_name << " at " << to_string(localization) << finl;

    const int number_of_component = (values.nb_dim() == 1 ? 0 : values.dimension(1));

	// compute the component names
	Noms units, component_names;
	assert(number_of_component <= 3);
	for (int index = 0; index < number_of_component; index++)
	  {
        units.add("");
        component_names.add(static_cast<char>('X' + index));
	  }
	
    Domaine domain;
    domain.nommer(domain_name);

    double temps_courant = mon_probleme->schema_temps().temps_courant();

    Nom nature = (number_of_component == 1) ? "scalar" : "vectorial";

    const int component_to_process = -1; // meaning that we always want all components

	Nom localization_string = to_string(localization);

    // For ELEMENT processing, we need to filter out virtual facettes (if requested):
	if (localization == Localization::Element && no_virtuals_)
	  {
        DoubleTab filtered_values;
        filter_out_array(values, filtered_values);
        postraiter_tableau(domain, units, component_names, component_to_process, temps_courant, field_name, localization_string, nature, filtered_values);
      }
	else
	  {
        postraiter_tableau(domain, units, component_names, component_to_process, temps_courant, field_name, localization_string, nature, values);
      }
  }

