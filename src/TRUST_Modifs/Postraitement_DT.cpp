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

#include <vector>
#include <iomanip>

#include <Postraitement_DT.h>
#include <SFichier.h>
#include <Operateur.h>
#include <Statistiques.h>

Implemente_instanciable(Postraitement_DT, "Postraitement_DT", Postraitement);

/**
 * @brief Reads the input stream and initializes the output file.
 *
 * This method reads the necessary parameters, removes the file extension
 * from `nom_fich_`, and writes the header information to the output file.
 *
 * @param is Input stream.
 * @return Entree& The modified input stream.
 */
Entree& Postraitement_DT::readOn(Entree& is)
{
  // read input has standard Postraitement
  Postraitement::readOn(is);

  // Remove file extension from nom_fich_ that was automatically added
  std::string file_name(nom_fich_);
  size_t last_dot_pos = file_name.find_last_of('.');
  if (last_dot_pos != std::string::npos)
    nom_fich_ = file_name.substr(0, last_dot_pos);

  // get problem
  const Probleme_base& problem = ref_cast(Probleme_base, mon_probleme.valeur());

  // Open file
  SFichier file;
  file.ouvrir(nom_fich(), ios::out);

  // Write description
  file << "File generated from Postraitement_DT class.\n\n";

  file << format("Description:\n", "\033[1m");
  file << "This file contains the time steps computed for each equations, has well has the minimum, maximum and effective time steps.\n";
  file << "All values are in seconds.\n\n";

  // Write equations type and name
  file << format("Equations [type and name]:\n", "\033[1m");
  for (int index = 0; index < problem.nombre_d_equations(); index++)
    {
      const Nom& equation_type = problem.equation(index).que_suis_je();
      const Nom& equation_name = problem.equation(index).le_nom();

      std::ostringstream oss;
      oss << "\t" << std::left << std::setw(47) << equation_type
          << std::setw(column_gap) << "";
      oss << std::left << std::setw(column_width) <<equation_name
          << std::setw(column_gap) << "\n";

      file << oss.str();
    }
  file << "\n";

  // print the color code used
  if (formatting_flag)
    {
      file << "\033[1mColor code:\n";

      // write
      file << formatter_minimum << "\tMinimum" << formatter_reset
           << ":   The minimum value among all time steps for the given time.\n"
           << formatter_maximum << "\tMaximum" << formatter_reset
           << ":   The maximum value among all time steps for the given time.\n"
           << formatter_effective << "\tEffective" << formatter_reset
           << ": The time step value actually used for computation at the given time."
           << "\033[0m\n\n";
    }

  // write the header
  if (formatting_flag)
    file << "\033[1m"; // bold

  file << write_text_column("Time");
  file << write_text_column("Duration");

  for (int index_equation = 0;
       index_equation < problem.nombre_d_equations(); index_equation++)
    {

      file << separator;

      const Equation_base& equation = ref_cast(Equation_base, problem.equation(index_equation));

      const Nom& equation_name = equation.le_nom();
      file << write_text_column(&(*equation_name));

      // also print each operators of the equation
      for (int index_operator = 0;
           index_operator < equation.nombre_d_operateurs(); index_operator++)
        {

          //const Nom& operator_name = equation.operateur(index_operator).type();
          std::string operator_name(typeid(equation.operateur(index_operator)).name());

          // remove the integers at the begining of operator_name
          size_t pos = 0;
          while (pos < operator_name.size() && std::isdigit(operator_name[pos]))
            ++pos;

          operator_name = operator_name.substr(pos);

          // then write it to the file
          std::string text = write_text_column(operator_name.c_str());
          file << "\033[0m" << text;

          if (formatting_flag)
            file << "\033[1m";
        }
    }

  file << separator;
  file << write_text_column("Minimum");

  file << separator;
  file << write_text_column("Maximum");

  file << separator;
  file << write_text_column("Effective");

  if (formatting_flag)
    file << "\033[0m"; // normal

  file << "\n";

  // close the file
  file.close();

  return is;
}

/**
 * @brief Prints the object to the output stream.
 *
 * This method currently does nothing and returns the output stream as is.
 *
 * @param os Output stream.
 * @return Sortie& The unchanged output stream.
 */
Sortie& Postraitement_DT::printOn(Sortie& os) const
{
  // do nothing
  return os;
}

/**
 * @brief Sets the parameters of the object.
 *
 * This method delegates parameter setting to the base class `Postraitement`.
 *
 * @param param Reference to the parameter object.
 */
void Postraitement_DT::set_param(Param& param)
{
  Postraitement::set_param(param);

  param.ajouter("column_width",       &column_width);
  param.ajouter("column_gap",         &column_gap);
  param.ajouter("number_of_decimals", &number_of_decimals);
  param.ajouter("formatting",         &formatting_flag);
}

/**
 * @brief Appends a new line to the output file with the estimated time steps.
 *
 * This method writes the current simulation time followed by the time step values
 * for each equation of the problem to the output file.
 */
void Postraitement_DT::postraiter(int)
{
  // get problem and time scheme
  const Probleme_base& problem = ref_cast(Probleme_base, mon_probleme.valeur());
  const Schema_Temps_base& time_scheme = ref_cast(Schema_Temps_base, problem.schema_temps());

  // get all time steps (equations, minimum, maximum and effective time steps)
  std::vector<double> time_steps;
  // and get all the time steps of each operator
  std::vector<std::vector<double>> time_steps_operators;

  for (int index_equation = 0;
       index_equation < problem.nombre_d_equations(); index_equation++)
    {

      const Equation_base& equation = ref_cast(Equation_base, problem.equation(index_equation));

      double dt_equation = equation.calculer_pas_de_temps();
      time_steps.push_back(dt_equation);

      // also compute the time step of each operators of the equation
      time_steps_operators.push_back(std::vector<double>());
      for (int index_operator = 0;
           index_operator < equation.nombre_d_operateurs(); index_operator++)
        {

          double dt_operator = equation.operateur(index_operator).calculer_pas_de_temps();
          time_steps_operators.back().push_back(dt_operator);
        }
    }

  time_steps.push_back(time_scheme.pas_temps_min());
  time_steps.push_back(time_scheme.pas_temps_max());

  double effective_time_step = problem.calculer_pas_de_temps();
  time_steps.push_back(effective_time_step);

  // find the minimum and maximum time step values
  auto [min_it, max_it] = std::minmax_element(time_steps.begin(), time_steps.end());

  // only the master process write in the file
  if (Process::je_suis_maitre())
    {

      // Open file
      SFichier file;
      file.ouvrir(nom_fich(), ios::app);

      // Get current simulation time and write it to the file
      double current_time = time_scheme.temps_courant();
      file << write_float_column(current_time);

      // Get the current computation time and compare with the previous one to get
      // the computation duration for this timestep
      double duration = Statistiques::get_time_now();
      file << write_float_column(duration - previous_duration);
      previous_duration = duration;

      // Write all time steps
      for (size_t index_equation = 0;
           index_equation < time_steps.size(); index_equation++)
        {

          file << separator;

          double time_step = time_steps[index_equation];

          // format differently the output if its a minimum or maximum value
          std::string formatter;
          if       (time_step == effective_time_step)
            formatter = formatter_effective;
          else if (time_step == *min_it)
            formatter = formatter_minimum;
          else if (time_step == *max_it)
            formatter = formatter_maximum;

          // write the value in the buffer
          std::string text = write_float_column(time_step);

          // write to the files with formatter
          file << format(text, formatter);

          // then write the operators time steps
          if (index_equation < time_steps_operators.size())
            {
              for (double time_step_operator: time_steps_operators[index_equation])
                {
                  text = write_float_column(time_step_operator);
                  file << format(text, formatter_operator);
                }
            }
        }

      file << "\n";

      // close the file
      file.close();
    }
}

/**
 * @brief apply or not the formatter to the string depending on the formatting_flag.
 *
 * @param input Input string.
 * @param formatter Formatter string.
 * @return std::string Formatted string.
 */
std::string Postraitement_DT::format(
  const std::string input, const std::string formatter) const
{

  if (formatting_flag)
    return formatter + input + formatter_reset;
  else
    return input;
}

/**
 * @brief Writes a floating point value to a character buffer.
 *
 * This function formats a floating point value into scientific notation if
 * greater than 1, ensuring a fixed column width with a trailing gap.
 *
 * @tparam Float Floating point type (e.g., float, double).
 * @param value Floating point value to format.
 * @return std::string The formatted string.
 */
template <class Float>
std::string Postraitement_DT::write_float_column(
  const Float value) const
{

  // Ensure Float is a floating-point type
  static_assert(std::is_floating_point<Float>::value, "write_float_column requires a floating-point type (float or double).");

  std::ostringstream oss;
  oss << std::left << std::scientific << std::setprecision(number_of_decimals)
      << std::setw(column_width) << value
      << std::setw(column_gap) << "";

  return oss.str();
}

/**
 * @brief Writes a text value to a character buffer.
 *
 * This function formats a text string into a fixed-width column with a trailing gap.
 *
 * @param text Text string to format.
 * @return std::string The formatted string.
 */
std::string Postraitement_DT::write_text_column(
  const char text[]) const
{

  std::ostringstream oss;
  oss << std::left << std::setw(column_width) << text
      << std::setw(column_gap) << "";

  return oss.str();
}
