
// 
// Hex viewer for qemu-esp32 flash data files or dumps
//

#include <nana/gui.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/checkbox.hpp>
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

#include <cstring>
#include <cinttypes>

#define SS(s) std::string(s)

#define STEP_LEN 2048

#ifndef QEMU_PATH
#define QEMU_PATH "/home/olof/play/calib/build"
#endif

#ifndef QEMU_MACHINE
#define QEMU_MACHINE "esp32"
#endif

#ifndef QEMU_FLASH_FILE
#define QEMU_FLASH_FILE "esp32flash.bin"
#endif

std::vector<std::string> gFlashFiles;
std::vector<int> gFlashOffsets;
std::string file_to_load;
std::vector<unsigned char> buffer;
bool is_click=false;
unsigned int gOffset = 0;



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
	// Create a temporary hidden owner form — nana's filebox needs a valid parent window
	form owner;
	owner.show();
	filebox fb(owner, true);
	fb.add_filter(("Text File"), ("*.bin"));
	fb.add_filter(("All Files"), ("*.*"));
	auto files = fb();
	owner.close();
	if (!files.empty())
	{
		std::cout << "Selected file:  " << files.front().string() << std::endl;
		return files.front().string();
	}
	return {};
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
	if (file_to_load.empty()) {
		std::cerr << "No file specified" << std::endl;
		buffer.clear();
		return;
	}
	std::ifstream file(file_to_load.c_str(), std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Cannot open file: " << file_to_load << std::endl;
		buffer.clear();
		return;
	}
	std::streamsize size = file.tellg();
	if (size <= 0 || static_cast<size_t>(size) > 64 * 1024 * 1024) {
		std::cerr << "Empty or invalid file (size=" << size << "): " << file_to_load << std::endl;
		buffer.clear();
		return;
	}
	file.seekg(0, std::ios::beg);
	buffer.resize(static_cast<size_t>(size));

	if (file.read((char *)&*buffer.begin(), size))
	{
		/* worked! */
	}
	return;
}

// ESP32 partition table entry (32 bytes)
struct esp_partition_entry {
	uint16_t magic;      // 0x50AA
	uint8_t  type;
	uint8_t  subtype;
	uint32_t offset;
	uint32_t size;
	char     label[16];
	uint32_t flags;
} __attribute__((packed));

static const char* partition_type_str(uint8_t type) {
	switch (type) {
		case 0x00: return "app";
		case 0x01: return "data";
		default:   return "unknown";
	}
}

static const char* partition_subtype_str(uint8_t type, uint8_t subtype) {
	if (type == 0x00) { // app
		switch (subtype) {
			case 0x00: return "factory";
			case 0x20: return "test";
			default:
				if ((subtype & 0xF0) == 0x10) return "ota";
				return "unknown";
		}
	} else if (type == 0x01) { // data
		switch (subtype) {
			case 0x00: return "ota";
			case 0x01: return "phy";
			case 0x02: return "nvs";
			case 0x03: return "coredump";
			case 0x04: return "nvs_keys";
			case 0x05: return "efuse";
			case 0x80: return "esphttpd";
			case 0x81: return "fat";
			case 0x82: return "spiffs";
			default:   return "unknown";
		}
	}
	return "unknown";
}

void add_listbox_from_flash(listbox& lsbox, unsigned int table_offset) {
	lsbox.append_header("Name");
	lsbox.append_header("Type");
	lsbox.append_header("SubType");
	lsbox.append_header("Offset");
	lsbox.append_header("Size");

	// Always add bootloader and partition table entries
	char pt_off_str[16];
	snprintf(pt_off_str, sizeof(pt_off_str), "0x%x", table_offset);
	lsbox.at(0).append({ SS("bootloader"), SS("app"), SS("boot"), SS("0x1000"), SS("0x7000") });
	lsbox.at(0).append({ SS("partition_table"), SS(""), SS(""), SS(pt_off_str), SS("0x1000") });

	if (table_offset + sizeof(esp_partition_entry) > buffer.size()) {
		std::cerr << "Partition table offset 0x" << std::hex << table_offset << " beyond file size" << std::endl;
		return;
	}

	for (int i = 0; i < 96; i++) {
		size_t pos = table_offset + i * sizeof(esp_partition_entry);
		if (pos + sizeof(esp_partition_entry) > buffer.size()) break;

		const esp_partition_entry *e = reinterpret_cast<const esp_partition_entry*>(&buffer[pos]);

		// End of table: magic 0xEBEB (MD5) or not 0x50AA
		if (e->magic == 0xEBEB || e->magic != 0x50AA) break;

		char off_str[16], size_str[16];
		snprintf(off_str, sizeof(off_str), "0x%x", e->offset);
		snprintf(size_str, sizeof(size_str), "0x%x", e->size);

		// Ensure label is null-terminated
		char label[17] = {};
		memcpy(label, e->label, 16);

		lsbox.at(0).append({
			SS(label),
			SS(partition_type_str(e->type)),
			SS(partition_subtype_str(e->type, e->subtype)),
			SS(off_str),
			SS(size_str)
		});

		gFlashOffsets.push_back(e->offset);
	}
}

class qemu_settings_form
	: public nana::form
{
public:
	qemu_settings_form();

	nana::checkbox chk_debugger_;
	nana::checkbox chk_display_;
	nana::checkbox chk_wifi_;
	nana::checkbox chk_serial_;
	nana::textbox  txt_extra_;
	nana::label    lbl_extra_;
	nana::button   run_;
	nana::button   cancel_;
};

std::unique_ptr<qemu_settings_form>  gSettings;

void run_qemu();

qemu_settings_form::qemu_settings_form()
	: nana::form(nana::API::make_center(450, 250))
{
	this->caption("QEMU Settings");

	chk_debugger_.create(*this, nana::rectangle(10, 10, 200, 25));
	chk_debugger_.caption("Wait for debugger (-S)");

	chk_display_.create(*this, nana::rectangle(10, 40, 200, 25));
	chk_display_.caption("Display window");
	chk_display_.check(true);

	chk_wifi_.create(*this, nana::rectangle(10, 70, 200, 25));
	chk_wifi_.caption("WiFi NIC");
	chk_wifi_.check(true);

	chk_serial_.create(*this, nana::rectangle(10, 100, 200, 25));
	chk_serial_.caption("Serial to stdio");
	chk_serial_.check(true);

	lbl_extra_.create(*this, nana::rectangle(10, 135, 80, 20));
	lbl_extra_.caption("Extra args:");

	txt_extra_.create(*this, nana::rectangle(90, 133, 250, 25));

	run_.create(*this, nana::rectangle(370, 40, 60, 25));
	run_.caption("Run");
	run_.events().click([]() {
		run_qemu();
		});

	cancel_.create(*this, nana::rectangle(370, 100, 60, 25));
	cancel_.caption("Cancel");

	cancel_.events().click([]() {
		gSettings->hide();
		});

	this->events().unload([](const arg_unload& arg) {
		gSettings.reset(nullptr);
	});
}


void run_qemu()
{
	static bool opt_debugger = false;
	static bool opt_display = true;
	static bool opt_wifi = true;
	static bool opt_serial = true;
	static std::string opt_extra;

	if (gSettings) {
		opt_debugger = gSettings->chk_debugger_.checked();
		opt_display = gSettings->chk_display_.checked();
		opt_wifi = gSettings->chk_wifi_.checked();
		opt_serial = gSettings->chk_serial_.checked();
		opt_extra = gSettings->txt_extra_.text();
	}

	const char *flash = file_to_load.empty() ? QEMU_FLASH_FILE : file_to_load.c_str();

	std::string cmd = std::string(QEMU_PATH) + "/qemu-system-xtensa"
		" -machine " + QEMU_MACHINE +
		" -drive file=" + flash + ",if=mtd,format=raw"
		" -parallel none"
		" -monitor none"
		" -global driver=timer.esp32.timg,property=wdt_disable,value=true"
		" -serial stdio";

	if (opt_display)
		cmd += " -display default,show-cursor=on";
	else
		cmd += " -nographic";

	if (opt_wifi)
		cmd += " -nic user,model=esp32_wifi,net=192.168.4.0/24,hostfwd=tcp::16555-192.168.4.1:80";

	if (!opt_serial)
		cmd.replace(cmd.find(" -serial stdio"), 14, "");

	if (opt_debugger)
		cmd += " -S";

	if (!opt_extra.empty())
		cmd += " " + opt_extra;

	cmd += " 2> log.txt ";

	std::cout << cmd << std::endl;
	std::system(cmd.c_str());
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
	if (buffer.empty() || gOffset >= buffer.size()) return;
	unsigned char *ptr = &*buffer.begin() + gOffset;
	std::string new_text = log_dump(ptr, buffer.size() - gOffset, 16);
	tbox.hide();
	tbox.reset(new_text, false);
	// Force redraw of line, (Should be done differently)
	tbox.show();
}



int main(int argc, char *argv[])
{
	using namespace nana;
	bool disable_draw = false;

	std::string csv_file;
	int partition_table_offset = -1;

	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "-p" && i + 1 < argc) {
			csv_file = argv[++i];
		} else if (std::string(argv[i]) == "-t" && i + 1 < argc) {
			partition_table_offset = strtol(argv[++i], NULL, 0);
		} else {
			file_to_load = argv[i];
		}
	}

	// Default: parse partition table from flash at 0x8000
	if (csv_file.empty() && partition_table_offset < 0) {
		partition_table_offset = 0x8000;
	}

	if (file_to_load.empty()) {
		printf("Usage: %s [flash_file] [-t partition_offset] [-p partitions.csv]\n", argv[0]);
		file_to_load = load_file_dialog();
	}

	form fm(API::make_center(860, 800), nana::appear::decorate<nana::appear::sizable>());
	fm.div("vert <dw weight=5%>"
		"<lsbox weight=20%>"
		"<<flash_btn><qemu_btn><stop_btn> weight=4%>"
		"<<line weight=40><marginr=20 width=90% tbox>>");

	//Define panel widgets for overview bar and line numbers.
	panel<true> overview(fm);
	panel<true> line(fm);
	textbox tbox(fm);

	API::window_cursor(fm, nana::cursor::arrow);
	API::window_cursor(overview, nana::cursor::hand);
	API::window_cursor(line, nana::cursor::arrow);

	if (!file_to_load.empty()) {
		reloadFile();
		if (!buffer.empty()) {
			setData(tbox);
		}
	}
	overview.drawing([&](paint::graphics& graph)
	{
		graph.rectangle(false, colors::black);
		int pos = 0;
		for (size_t i = 0; i < buffer.size() && pos < (int)graph.width(); i+=STEP_LEN) {
			if ((buffer[i]!=0xff) && (buffer[i]!=0x0)) {
				graph.line(point(pos, 0), point(pos, graph.height()), colors::blue);
			} else if (buffer[i]==0x0) {
				graph.line(point(pos, 0), point(pos, graph.height()), colors::green);
			} else {
				graph.line(point(pos, 0), point(pos, graph.height()), colors::white);
			}
			pos++;
		}
		graph.rectangle(rectangle{(int)(gOffset / STEP_LEN), 0, 4, (unsigned)graph.height()}, true, colors::red);
	});

	listbox lsbox(fm);
	if (partition_table_offset >= 0 && !buffer.empty()) {
		add_listbox_from_flash(lsbox, partition_table_offset);
	} else if (!csv_file.empty()) {
		add_listbox_items(lsbox, csv_file);
	} else {
		add_listbox_items(lsbox, "qemu_partitions.csv");
	}

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
		API::refresh_window(overview);
	});

    inputbox ibox(fm, "Goto address (hex: 0x1000, decimal: 4096)", "Goto");
    inputbox::text goto_addr{"Address", "0xf000"};

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
		if (' ' == _arg.key)
		{
			gOffset+=16;
			while (gOffset<buffer.size() && ( buffer[gOffset] == 0 || buffer[gOffset] == 0xff)) {
				gOffset++;
			} 
			setData(tbox);
			//Ignore the input. the member ignore only valid in key_char.
			_arg.ignore = true;
		}
		if ('g' == _arg.key)
		{
			if (ibox.show(goto_addr)){
				 auto s = goto_addr.value();
				 gOffset = strtol(s.c_str(), NULL, 0);
			}
			setData(tbox);
			_arg.ignore = true;
		}
	});

	overview.events().mouse_down([&](const nana::arg_mouse& a_m)
		{
			is_click = true;
			gOffset = a_m.pos.x * STEP_LEN;
			setData(tbox);
			API::refresh_window(overview);
		});

	overview.events().mouse_up([&]()
		{
			is_click = false;
		});

	overview.events().mouse_move([&](const nana::arg_mouse& a_m)
		{
			if (is_click) {
				gOffset = a_m.pos.x * STEP_LEN;
				setData(tbox);
				API::refresh_window(overview);
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
	line.drawing([&](paint::graphics& graph)
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
	btn2.events().click([&](const nana::arg_click& a_m) {
			if (a_m.mouse_args && a_m.mouse_args->shift) {
				if (!gSettings) {
					gSettings = std::make_unique<qemu_settings_form>();
				}
				if (gSettings) gSettings->show();
			} else {
				run_qemu();
			}
		});
		

	button btn3(fm, "Stop qemu");
	btn3.events().click(stop_qemu);


	fm["dw"] << overview;
	fm["lsbox"] << lsbox;
	fm["flash_btn"] << btn;
	fm["qemu_btn"] << btn2;
	fm["stop_btn"] << btn3;
	fm["line"] << line;
	fm["tbox"] << tbox;

	fm.collocate();

	fm.events().unload([](const arg_unload& arg) {
		//arg.cancel = true;
		if (gSettings) {
			gSettings->hide();
			gSettings.reset(nullptr);
		}
		});
	fm.show();
	exec();
}

