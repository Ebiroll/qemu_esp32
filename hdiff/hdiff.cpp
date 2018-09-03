
// 
// Hex viewer for qemu-esp32 flash data files or dumps
//

#include <nana/gui.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/widgets/listbox.hpp>

#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/place.hpp>
#include <iostream>
#include <fstream>

//#include <sstream>

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

inline char hdigit(int n) { return "0123456789abcdef"[n & 0xf]; };

#define LEN_LIMIT 256
#define SUBSTITUTE_CHAR '`'

static const char* dumpline(char*dest, int linelen, const char*src, const char*srcend)
{
	if (src >= srcend) {
		return 0;
	}
	int i;
	unsigned long s = (unsigned long)src;
	for (i = 0; i < 8; i++) {
		//dest[i] = hdigit(s>>(28-i*4));
		dest[i] = ' ';
	}
	dest[8] = ' ';
	dest += 9;
	for (i = 0; i < linelen / 4; i++) {
		if (src + i < srcend) {
			dest[i * 3] = hdigit(src[i] >> 4);
			dest[i * 3 + 1] = hdigit(src[i]);
			dest[i * 3 + 2] = ' ';
			dest[linelen / 4 * 3 + i] = src[i] >= ' ' && src[i] < 0x7f ? src[i] : SUBSTITUTE_CHAR;
		}
		else {
			dest[i * 3] = dest[i * 3 + 1] = dest[i * 3 + 2] = dest[linelen / 4 * 3 + i] = ' ';
		}
	}
	return src + i;
}


std::string log_dumpf(const char*fmt, const void*addr, int len, int linelen, textbox &tb)
{
	std::string ret;
#if LEN_LIMIT
	if (len > linelen*LEN_LIMIT) {
		len = linelen * LEN_LIMIT;
	}
#endif
	linelen *= 4;
	static char _buf[4096];
	char*buf = _buf;
	buf[linelen] = 0;
	const char*start = (char*)addr;
	const char*cur = start;
	const char*end = start + len;
	while (!!(cur = dumpline(buf, linelen, cur, start + len))) {
		std::string line = std::string(buf) + "\n";
		ret += line;
	}
	return ret;
}

std::string log_dump(const void*addr, int len, int linelen, textbox &tb)
{
	return(log_dumpf("%s\n", addr, len, linelen, tb));
}

int main(int argc, char *argv[])
{
	using namespace nana;
	bool disable_draw = false;

	form fm(API::make_center(860, 800), nana::appear::decorate<nana::appear::sizable>());
	fm.div("vert <lsbox weight=20%>"
		"<<line weight=40><marginr=20 width=90% tbox>>");
	//form fm(API::make_center(800, 800),nana::appear::decorate<nana::appear::sizable>());
	//fm.div("horizontal <lsbox>"
	//       "margin=5 <line weight=15><marginr=100 weight=30 tbox>");

	//Define a panel widget to draw line numbers.
	panel<true> line(fm);

	std::vector<char> buffer;
	std::streamsize size;
	int offset = 0;

	textbox tbox(fm);

	if (argc == 1) {
		printf("Usage %s <file1> <file2> -p partition.csv\n", argv[0]);
	}

	if (argc > 1) {
		std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
		size = file.tellg();
		file.seekg(0, std::ios::beg);
		buffer.resize(size);

		if (file.read(buffer.data(), size))
		{
			std::string new_text = log_dump(&(*buffer.begin()), size, 16, tbox);
			tbox.append(new_text, false);
			/* worked! */
		}
		else
		{
			return 0;
		}
	}

	listbox lsbox(fm); // , rectangle{ 10, 10, 80, 100 }
	add_listbox_items(lsbox);

	lsbox.events().selected([&](const nana::arg_listbox &event) {
		//API::refresh_window(line);
		auto test = event.item.text(3);
		if (event.item.selected()) {
			std::cout << "Click " << test << "\n";
			offset = std::strtol(test.c_str(), NULL, 16);
			char *ptr = &*buffer.begin() + offset;
			std::string new_text = log_dump(ptr, size, 16, tbox);
			tbox.hide();
			tbox.reset(new_text, false);
			// Force redraw of line, (Should be done differently)
			tbox.show();

			//tbox.set_highlight("test", colors::dark_blue, colors::yellow);
			//tbox.set_keywords("test", false, true, { "password", "ssid", "ESP_OK" });

			//tbox.caret_pos(upoint(0, 10));
		}
	});


	tbox.typeface(nana::paint::font("monospace", 10, true));
	//tbox.line_wrapped(true); //Add this line and input a very very long line of text, give it a try.

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
		if (disable_draw)
			return;
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
			char hex_buff[16];

			sprintf(hex_buff, "0x%04x", offset + 16 * (pos.y)); // gives 12ab

			auto line_num = std::string(hex_buff);
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


	fm["lsbox"] << lsbox;
	fm["line"] << line;
	fm["tbox"] << tbox;
	fm.collocate();

	fm.show();
	exec();
}