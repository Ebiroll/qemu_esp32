
// 
// Hex viewer for qemu-esp32 flash data files or dumps
//

#include <nana/gui.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/button.hpp>

#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/place.hpp>
#include <iostream>
#include <fstream>

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>


using namespace nana;

#define SS(s) std::string(s)

#define STEP_LEN 2048

std::vector<std::string> gFlashFiles;
std::vector<int> gFlashOffsets;
std::string file_to_load;
std::vector<unsigned char> buffer;
bool is_click=false;
unsigned int gOffset = 0;


/*
class goto_form 
        : public nana::form 
 { 
    public: 
    goto_form(); 
  private: 
    void _m_pick_door  (const nana::arg_click& ei); 
  private: 
    nana::label  label_; 
 }; 
*/

class CSVRow
{
public:
	std::string const& operator[](std::size_t index) const
	{
		return m_data[index];
	}
	std::size_t size() const
	{
		return m_data.size();
	}
	void readNextRow(std::istream& str)
	{
		std::string         line;
		std::getline(str, line);

		std::stringstream   lineStream(line);
		std::string         cell;

		m_data.clear();
		while (std::getline(lineStream, cell, ','))
		{
			cell = cell.erase(0, cell.find_first_not_of(" \t"));
			cell = cell.erase(cell.find_last_not_of(" \t") + 1);
			m_data.push_back(cell);
		}
		// This checks for a trailing comma with no data after it.
		if (!lineStream && cell.empty())
		{
			// If there was a trailing comma then add an empty element.
			m_data.push_back("");
		}
	}
private:
	std::vector<std::string>    m_data;
};

std::istream& operator>>(std::istream& str, CSVRow& data)
{
	data.readNextRow(str);
	return str;
}

void add_listbox_items(listbox& lsbox,std::string csv_filename) {

	std::ifstream       file(csv_filename.c_str());

	//Create two columns
	lsbox.append_header("Name");
	lsbox.append_header("Type");
	lsbox.append_header("SubType");
	lsbox.append_header("Offset");
	lsbox.append_header("Size");
	lsbox.append_header("File");

	lsbox.at(0).append({ SS("bootloader"), SS("app"), SS("nvs"), SS("0x1000"),  SS("0x1000") });
	lsbox.at(0).append({ SS("partition"), SS("par"), SS("nvs"), SS("0x8000"),  SS("0x1000") });

	if (!file.is_open())
	{

		//Then append items
		lsbox.at(0).append({ SS("nvs"), SS("data"), SS("nvs"), SS("0x9000"),  SS("0x6000") });
		lsbox.at(0).append({ SS("phy_init"), SS("data"), SS("phy"), SS("0xf000"),  SS("0x1000") });
		lsbox.at(0).append({ SS("factory"), SS("app"), SS("factory"), SS("0x10000"),  SS("0x170000") });
		lsbox.at(0).append({ SS("spiffs"), SS("data"), SS("spiffs"), SS("0x180000"),  SS("0x80000") });
	} else {
		CSVRow              row;
		while (file >> row)
		{
			if (row.size() > 0) {
				if (row[0].at(0) != '#') {
					std::string filename;
					if(row.size()>4) {
						filename=row[5];
						gFlashFiles.push_back(filename);
						int off; //=atoi(row[2]);
						off=strtol(row[3].c_str(), NULL, 16);
						gFlashOffsets.push_back(off);
					}
					lsbox.at(0).append({row[0],row[1],row[2],row[3],row[4],filename});
				}
			}
		}

	}
}

inline char hdigit(int n) { return "0123456789abcdef"[n & 0xf]; };

#define LEN_LIMIT 512
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


std::string log_dumpf(const char*fmt, const void*addr, int len, int linelen)
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

std::string log_dump(const void*addr, int len, int linelen)
{
	return(log_dumpf("%s\n", addr, len, linelen));
}

std::string load_file_dialog() {
	filebox fb(0, true);
	fb.add_filter(("Text File"), ("*.bin"));
	fb.add_filter(("All Files"), ("*.*"));
	auto files = fb();
	if (!files.empty())
	{
		std::cout << "Selected file:  " << files.front().string() << std::endl;
	}
	return(files.front().string());
}

void merge_flash(const char *binfile,const char *flashfile,int flash_pos)
{
    FILE *fbin;
    FILE *fflash;
    unsigned char *tmp_data;

    int file_size=0;
    int flash_size=0;

    fbin = fopen(binfile, "rb");
    if (fbin == NULL) {
        printf("   Can't open '%s' for reading.\n", binfile);
		return;
	}

    if (fseek(fbin, 0 , SEEK_END) != 0) {
        printf("   Can't seek end of '%s'.\n", binfile);
       /* Handle Error */
    }
    file_size = ftell(fbin);
    if (fseek(fbin, 0 , SEEK_SET) != 0) {
      /* Handle Error */
    }

    fflash  = fopen(flashfile, "rb+");
    if (fflash == NULL) {
        printf("   Can't open '%s' for writing.\n", flashfile);
        return;
    }
    if (fseek(fflash, 0 , SEEK_END) != 0) {
          printf("   Can't seek end of '%s'.\n", flashfile);
       /* Handle Error */
    }
    flash_size = ftell(fflash);
    rewind(fflash);
    fseek(fflash,flash_pos,SEEK_SET);


    tmp_data=(unsigned char *)malloc((1+file_size)*sizeof(char));

    if (file_size<=0) {
      printf("Not able to get file size %s",binfile);
    }

    int len_read=fread(tmp_data,sizeof(char),file_size,fbin);    
    int len_write=fwrite(tmp_data,sizeof(char),file_size,fflash);

    if (len_read!=len_write) {
      printf("Not able to merge %s, %d bytes read,%d to write,%d file_size\n",binfile,len_read,len_write,file_size);
    }

    fclose(fbin);
    if (fseek(fflash, 0x3E8000*4 , SEEK_SET) != 0) {
    }
    fclose(fflash);
    free(tmp_data);
}

void reloadFile() {
	std::streamsize size;
	std::ifstream file(file_to_load.c_str(), std::ios::binary | std::ios::ate);
	size = file.tellg();
	file.seekg(0, std::ios::beg);
	buffer.resize(size);

	if (file.read((char *)&*buffer.begin(), size))
	{
		/* worked! */
	}
	return;
}

void run_qemu()
{
	//std::system("pwd"); 
	std::string launch="~/qemu_espressif/xtensa-softmmu/qemu-system-xtensa -nographic -s \
    -machine esp32 \
    -drive file=" + file_to_load + ",if=mtd,format=raw &";
	std::cout << launch << std::endl;
	std::system(launch.c_str());
}

void stop_qemu() {
	std::system("killall -9 qemu-system-xtensa");
	std::cout << "\n\n" << std::endl;
}


void qemu_flash()
{
	std::system("pwd");
	std::cout << "flash" << std::endl;
	for(unsigned int i=0;i<gFlashFiles.size(); i++) {
		std::cout <<  gFlashFiles[i] << " " << gFlashOffsets[i] << "\n";
		merge_flash(gFlashFiles[i].c_str(),file_to_load.c_str(),gFlashOffsets[i]);
	};

	reloadFile();
}


void setData(textbox &tbox) {
	unsigned char *ptr = &*buffer.begin() + gOffset;
	std::string new_text = log_dump(ptr, buffer.size(), 16);
	tbox.hide();
	tbox.reset(new_text, false);
	// Force redraw of line, (Should be done differently)
	tbox.show();
}



int main(int argc, char *argv[])
{
	using namespace nana;
	bool disable_draw = false;

	form fm(API::make_center(860, 800), nana::appear::decorate<nana::appear::sizable>());
	fm.div("vert <dw weight=5%>"
		"<lsbox weight=20%>"
		"<<flash_btn><qemu_btn><stop_btn> weight=4%>"
		"<<line weight=40><marginr=20 width=90% tbox>>");
	//form fm(API::make_center(800, 800),nana::appear::decorate<nana::appear::sizable>());
	//fm.div("horizontal <lsbox>"
	//       "margin=5 <line weight=15><marginr=100 weight=30 tbox>");

	//Define a panel widget to draw line numbers.
	panel<true> line(fm);
	textbox tbox(fm);

	if (argc == 1) {
		printf("Usage %s <file1> -p qemu_partition.csv\n", argv[0]);
		file_to_load= load_file_dialog();
	} else {
		file_to_load = std::string(argv[1]);
	}

	if (file_to_load.size() > 1) {
		reloadFile();
		setData(tbox);
	}
	drawing dw(fm);
	dw.draw([&](paint::graphics& graph)
	{
		int pos = 0;
		for (int i = 0; i < buffer.size() && pos < 1024; i+=STEP_LEN) {
			if ( buffer[i]!=0xff) {
				graph.line(point(pos, 0), point(pos, 20), colors::blue);
			} else {
				graph.line(point(pos, 0), point(pos, 20), colors::white);
			}
			pos++;
		}
		graph.rectangle(rectangle{(int) (gOffset /STEP_LEN), 0, 4, 20 }, true, colors::red);

	});

	listbox lsbox(fm); // , rectangle{ 10, 10, 80, 100 }
	add_listbox_items(lsbox,"qemu_partitions.csv");

	lsbox.events().selected([&](const nana::arg_listbox &event) {
		//API::refresh_window(line);
		auto test = event.item.text(3);
		if (event.item.selected()) {
			//std::cout << "Click " << test << "\n";
			gOffset = std::strtol(test.c_str(), NULL, 16);
			setData(tbox);

			//tbox.set_highlight("test", colors::dark_blue, colors::yellow);
			//tbox.set_keywords("test", false, true, { "password", "ssid", "ESP_OK" });
			//tbox.caret_pos(upoint(0, 10));
		}
		dw.update();
	});

    inputbox ibox(fm, "Goto adress.", "title");
    inputbox::integer integer{"goto", 0, 0, 1024*1024*4, 512};

	tbox.events().key_char([&](const nana::arg_keyboard& _arg) {
		if ('n' == _arg.key)
		{
			gOffset = gOffset + 512;
			setData(tbox);
			//Ignore the input. the member ignore only valid in key_char.
			_arg.ignore = true;
		}
		if ('p' == _arg.key)
		{
			if (gOffset>512) { 
				gOffset= gOffset - 512;
			} else {
				gOffset=0;
			}
			setData(tbox);
			//Ignore the input. the member ignore only valid in key_char.
			_arg.ignore = true;
		}
		if ('g' == _arg.key)
		{

			if (ibox.show(integer)){
				 auto n = integer.value();
				 gOffset= n;
			}
			setData(tbox);
			_arg.ignore = true;
		}
	});

	fm.events().click([&](const nana::arg_click& a_m)
		{
			is_click = true;
		});

	fm.events().mouse_down([&]()
		{
			is_click = true;
		});

	fm.events().mouse_up([&]()
		{
			is_click = false;
		});

	fm.events().mouse_move([&](const nana::arg_mouse& a_m)
		{
			//std::cout << a_m.pos.x << "\n";
			if (is_click) {
				gOffset = a_m.pos.x * STEP_LEN;
				setData(tbox);
				dw.update();
			}
			nana::API::refresh_window(fm);
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

			sprintf(hex_buff, "0x%04x", gOffset + 16 * (pos.y)); // gives 12ab

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

	button btn(fm);
	btn.caption("Flash new binary");
	btn.events().click(qemu_flash);
	
	button btn2(fm, "Start qemu");
	btn2.events().click(run_qemu);

	button btn3(fm, "Stop qemu");
	btn3.events().click(stop_qemu);


	//fm["dw"] << dw;
	fm["lsbox"] << lsbox;
	fm["flash_btn"] << btn;
	fm["qemu_btn"] << btn2;
	fm["stop_btn"] << btn3;
	fm["line"] << line;
	fm["tbox"] << tbox;

	fm.collocate();

	fm.show();
	exec();
}