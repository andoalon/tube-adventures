#include <tinyxml2/tinyxml2.h>

#include <cassert>
#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>

#ifdef TUBE_ADVENTURES_HAVE_QT
#	include "mainwindow.hh"
#endif

using namespace std::literals;
using namespace std::literals::string_view_literals;

int main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[])
{
	constexpr char filename[] = "../../data/TUBE-ADVENTURES/TA01 yVebIlvkOnU.xml";

	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(filename) != tinyxml2::XMLError::XML_SUCCESS)
	{
		std::error_code filename_error;
		auto full_filename = std::filesystem::weakly_canonical(filename, filename_error);

		if (filename_error)
		{
			std::clog << "Can't get absolute filename of path: \"" << filename << "\". Error: " << filename_error.message() << '\n';
			full_filename = filename;
		}

		std::clog << "Error parsing file: " << full_filename << ". Error type: " << doc.ErrorName() << ". Error: " << doc.ErrorStr() << '\n';
		return 1;
	}

	const auto check_error = [&doc](const void* element, const char* expression)
	{
		assert(expression != nullptr);

		if (element != nullptr)
			return;

		std::clog << "Error when running: " << expression << '\n';
		std::exit(2);
	};

#define run_and_check(expression) [&]{ const auto expression_result = (expression); check_error(expression_result, #expression); return expression_result; }()

	const tinyxml2::XMLElement* const document = run_and_check(doc.FirstChildElement("document"));

	const tinyxml2::XMLElement* const annotations = run_and_check(document->FirstChildElement("annotations"));

	std::cout << annotations->Value() << '\n';

	const char annotation_name[] = "annotation";
	for (const tinyxml2::XMLElement* annotation = run_and_check(annotations->FirstChildElement(annotation_name)); annotation != annotations->LastChild(); annotation = annotation->NextSiblingElement(annotation_name))
	{
		assert(annotation != nullptr);
		std::cout << "  " << annotation->Value() << '\n';
		
		const tinyxml2::XMLElement* const annotation_text = run_and_check(annotation->FirstChildElement("TEXT"));

		std::string text;
		text.reserve(2);

		text += '"';
		text += run_and_check(annotation_text->GetText());
		text += '"';

		for (std::size_t i = 0; i < text.size(); )
		{
			const char ch = text[i];

			if (ch != '\n')
			{
				++i;
				continue;
			}

			text.replace(i, 1, "\\n"sv);
			i += 2;
		}

		std::cout << "    " << text << '\n';

		if (const tinyxml2::XMLElement* const action = annotation->FirstChildElement("action"); action != nullptr)
		{
			if (const tinyxml2::XMLAttribute* const action_type = action->FindAttribute("type"); action_type != nullptr && run_and_check(action_type->Value()) == "openUrl"sv)
			{
				const tinyxml2::XMLElement* const url = run_and_check(action->FirstChildElement("url"));
				const tinyxml2::XMLAttribute* const url_value = run_and_check(url->FindAttribute("value"));

				const char* const url_c_str = run_and_check(url_value->Value());
				const std::string_view url_text = url_c_str;
				std::cout << "    " << std::quoted(url_text) << '\n';
			}
		}
	}

#ifdef TUBE_ADVENTURES_HAVE_QT
	QApplication a(argc, argv);

	MainWindow w;
	w.show();

	const auto retval = a.exec();

	return retval;
#else
	return 0;
#endif // TUBE_ADVENTURES_HAVE_QT
}
