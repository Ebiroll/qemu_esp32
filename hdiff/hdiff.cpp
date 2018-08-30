
//Nana's example
//This example illustrates how to show the line number of textbox.
//Nana 1.5.4 is required

#include <nana/gui.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/widgets/listbox.hpp>

#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/place.hpp>
#include <iostream>

using namespace nana;

#define SS(s) std::string(s)

void add_listbox_items(listbox& lsbox) {
	//Create two columns
	lsbox.append_header("Name");
	lsbox.append_header("Type");
	lsbox.append_header("SubType");
	lsbox.append_header("Offset");
	lsbox.append_header("Size");
	lsbox.append_header("Flags");

	//Then append items
	lsbox.at(0).append({ SS("nvs"), SS("data"), SS("nvs"), SS("0x9000"),  SS("0x6000") });
	lsbox.at(0).append({ SS("phy_init"), SS("data"), SS("phy"), SS("0xf000"),  SS("0x1000") });
	lsbox.at(0).append({ SS("factory"), SS("app"), SS("factory"), SS("0x10000"),  SS("0x170000") });
	lsbox.at(0).append({ SS("spiffs"), SS("data"), SS("spiffs"), SS("0x180000"),  SS("0x80000") });


	//Create a new category
	//lsbox.append("Category 1");

	//Append items for category 1
	//lsbox.at(1).append({ std::string("Hello4"), std::string("World4") });
	//lsbox.at(1).append({ std::string("Hello5"), std::string("World5") });
	//lsbox.at(1).append({ std::string("Hello6"), std::string("World6") });
	//lsbox.at(1).append({ std::string("Hello7"), std::string("World7") });



}

int main()
{
	using namespace nana;

	form fm(API::make_center(800, 800),nana::appear::decorate<nana::appear::sizable>());
    fm.div("horizontal <lsbox>"
           "margin=5 <line weight=15><marginr=100 weight=30 tbox>");

	//Define a panel widget to draw line numbers.
	panel<true> line(fm);

	listbox lsbox(fm, rectangle{ 10, 10, 80, 100 });
	add_listbox_items(lsbox);

    lsbox.events().selected([&](const nana::arg_listbox &event){
		//API::refresh_window(line);
		auto test=event.item.text(3);
  		std::cout << "Click " << test << "\n"; 
	});


#if 1
	textbox tbox(fm);

	tbox.append("No where to select the fox where i am!",true);
	tbox.typeface(nana::paint::font("", 20, true));
	//tbox.line_wrapped(true); //Add this line and input a very very long line of text, give it a try.

    tbox.set_highlight("nisse", colors::dark_blue, colors::yellow);
    tbox.set_keywords("sqlrev", false, true, { "select", "from", "where"});

	//Layout management.
	place plc(fm);

	//Define two fields, the weight of line field will be changed.
	//fm.div("margin=5 <line weight=15><text>");

	//fm["line"] << line;
	//fm["text"] << tbox;
	//fm.collocate();

	//Draw the line numbers
	drawing{ line }.draw([&](paint::graphics& graph)
	{
		auto line_px = tbox.line_pixels();
		if (0 == line_px)
			return;

		//Returns the text position of each line that currently displays on screen.
		auto text_pos = tbox.text_position();

		//Textbox has supported smooth scrolling since 1.5. Therefore it would only render
		//the lower part of top displayed line if it is scrolled less than a line height.
		//
		//tbox.content_origin().y % line_px, get the height which is scrolled less than
		//a line height.
		int top = tbox.text_area().y - tbox.content_origin().y % line_px;
		int right = static_cast<int>(graph.width()) - 5;

		for (auto & pos : text_pos)
		{
			auto line_num = std::to_wstring(pos.y + 1);
			auto pixels = graph.text_extent_size(line_num).width;

			//Check if the panel widget is not enough room to display a line number
			if (pixels + 5 > graph.width())
			{
				//Change the weight of 'line' field.
				fm.get_place().modify("line", ("weight=" + std::to_string(pixels + 10)).c_str());
				fm.collocate();
				return;
			}

			//Draw the line number
			graph.string({ right - static_cast<int>(pixels), top }, line_num);

			top += line_px;
		}
	});

	//When the text is exposing in the textbox, refreshs
	//the panel widget to redraw new line numbers.
	tbox.events().text_exposed([&line]
	{
		API::refresh_window(line);
	});


    fm["lsbox"]  << lsbox;
	fm["line"]  << line;
    fm["tbox"]   << tbox;
    fm.collocate();
#endif
	fm.show();
	exec();
}