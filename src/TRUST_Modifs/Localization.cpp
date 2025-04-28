#include <Localization.h>

#include <string>
#include <ostream>

std::string to_string(const Localization& localization) {
	switch (localization) {
		case (Localization::Element):            return "ELEM";
		case (Localization::Vertex):             return "SOM";
		case (Localization::Face):               return "Face";
		case (Localization::FaceI):              return "FaceI";
		case (Localization::FaceJ):              return "FaceJ";
		case (Localization::FaceK):              return "FaceK";
		case (Localization::ConnectedComponent): return "SOM"; // appear as SOM in output files
		case (Localization::Unknown):            return "Unknown";
		case (Localization::Count):              return "<COUNT>";
		default:                                 return "<INVALID LOCALIZATION>";
	}
}

std::ostream& operator<<(std::ostream& os, const Localization& localization) {
	return os << to_string(localization);
}
