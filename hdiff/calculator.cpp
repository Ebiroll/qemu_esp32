/**
 *  \file calculator.cpp
 *  \brief Nana Calculator
 *	Nana 1.3 and C++11 is required.
 *	This is a demo for Nana C++ Library.
 *	It creates an intermediate level graphical calculator with little code.
 */

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/place.hpp>
#include <forward_list>
#include <map>
#include <cassert>

//#include <nana/paint/graphics.hpp"

#include <iostream>
#include <chrono>

#include <thread>

using namespace nana;

// workaround insufficiency in VS2013.
#if defined(_MSC_VER) && (_MSC_VER < 1900)	//VS2013
	const std::string plus_minus(to_utf8(L"\u00b1")  ;   // 0xB1    u8"\261"
#else
	const std::string plus_minus( u8"\u00b1" );
#endif

struct stateinfo
{
	enum class state{init, operated, assigned};

	state	       opstate{ state::init };
    std::string    operation;
	double         oprand { 0 };
	double         outcome{ 0 };
	label        & procedure;
	label        & result;

	stateinfo(label& proc, label& resl)
		: operation("+"),  procedure(proc), result(resl)
	{	}
};

void numkey_pressed(stateinfo& state, const arg_click& arg)
{
	if(state.opstate != stateinfo::state::init)
	{
		if(state.opstate == stateinfo::state::assigned)
		{
			state.outcome = 0;
			state.operation = "+";			
		}
		state.result.caption("");
		state.opstate = stateinfo::state::init;
	}

	std::string rstr = state.result.caption();
	if(rstr == "0")	rstr.clear();

	std::string d = API::window_caption(arg.window_handle);
	if(d == ".")
	{
		if(rstr.find('.') == rstr.npos)
			state.result.caption(rstr.size() ? rstr + d : std::string("0."));
	}
	else
		state.result.caption(rstr + d);
}

void opkey_pressed(stateinfo& state, const arg_click& arg)
{
	std::string d = API::window_caption(arg.window_handle) ;
	if("C" == d)
	{
		state.result.caption("0");
		state.procedure.caption("");
		state.opstate = stateinfo::state::init;
		state.outcome = 0;
		state.operation = "+";
		return;
	}
	else if( plus_minus == d)
	{
		auto s = state.result.caption();
		if(s.size())
		{
			if(s[0] == '-')
				s.erase(0, 1);
			else
				s.insert(0, 1, '-');

			if(state.opstate == stateinfo::state::assigned)
			{
				state.outcome = -state.outcome;
				state.operation = "=";
			}

			state.result.caption(s);
			state.opstate = stateinfo::state::init;
		}
		return;
	}
	else if("%" == d)
	{
		auto s = state.result.caption();
		if(s.size())
		{
			double d = std::stod(s);
			d = state.outcome * d / 100;
			state.result.caption(std::to_string(d));
			state.opstate = stateinfo::state::init;
		}
		return;			
	}
	else if(state.opstate == stateinfo::state::operated)
		return;

	std::string oprandstr = state.result.caption();
	if(0 == oprandstr.size()) oprandstr = '0';

	std::string pre_operation = state.operation;
	std::string proc;
	if("=" != d)
	{
		state.operation = d;
		if(state.opstate != stateinfo::state::assigned)
			state.oprand = std::stod(oprandstr);
		else
			pre_operation = "=";

		proc =  state.procedure.caption()  + oprandstr ;
		if(("X" == d || "/" == d) && (proc.find_last_of("+-") != proc.npos))
		{
			proc.insert(0, "(");
			(( proc += ") " )  += d) += " ";
		}
		else
			((proc += " ") += d) += " ";

		state.opstate = stateinfo::state::operated;
	}
	else
	{
		if(state.opstate == stateinfo::state::init)
			state.oprand = std::stod(oprandstr);

		state.opstate = stateinfo::state::assigned;
	}

	switch(pre_operation[0])
	{
	case '+': 	state.outcome += state.oprand; 		break;
	case '-':	state.outcome -= state.oprand;		break;
	case 'X':	state.outcome *= state.oprand;		break;
	case '/':	state.outcome /= state.oprand;		break;
	}

	state.procedure.caption(proc);

	std::string outstr = std::to_string(state.outcome);
	while(outstr.size() && ('0' == outstr.back()))
		outstr.pop_back();
	
	if(outstr.size() && (outstr.back() == '.'))
		outstr.pop_back();
	if( outstr.empty() ) outstr += '0';
	state.result.caption(outstr);
}


int main()
{
	form fm;
	fm.caption(("Calculator"));
	
	//Use class place to layout the widgets.
	place place(fm);
	place.div(	"vert<procedure weight=10%><result weight=15%>"
                "<weight=2><opkeys margin=2 grid=[4, 5] gap=2 collapse(0,4,2,1)>");

	label procedure(fm), result(fm);

	//Make the label right aligned.
	procedure.text_align(nana::align::right);
	result.text_align(nana::align::right);
	result.typeface(nana::paint::font("amiga4ever.ttf", 14, true));

	place["procedure"] << procedure;
	place["result"] << result;

	stateinfo state(procedure, result);

	std::forward_list<button> op_keys;
	std::map<char,button*> bts;

	char keys[] = "Cm%/789X456-123+0.="; // \261
	paint::font keyfont("amiga4ever pro.ttf", 10, true);

	for (auto key : keys)
	{
		std::string Key;
		if (key == 'm')
			Key = plus_minus;  
    	else
			Key = std::string(1, key);

		op_keys.emplace_front(fm.handle());
		auto & key_btn = op_keys.front();
		bts[key]=&key_btn;

		key_btn.caption(Key);
		key_btn.typeface(keyfont);

		if ('=' == key)
		{
			key_btn.bgcolor(color_rgb(0x7ACC));
			key_btn.fgcolor(color_rgb(0xFFFFFF));
		}
		place["opkeys"] << key_btn;

		//Make event answer for keys.
		key_btn.events().click([key, &state](const arg_click& arg)
		{
			if (('0' <= key && key <= '9') || ('.' == key))
				numkey_pressed(state, arg);
			else
				opkey_pressed(state, arg);
		});
	}

	place.collocate();
	fm.show();
	exec(

#ifdef NANA_AUTOMATIC_GUI_TESTING
		
		1, 1, [&bts, &result ]()
	{
		click(*bts['2']); Wait( 1);
		click(*bts['+']); Wait( 1);
		click(*bts['2']); Wait( 1);

		click(*bts['=']);


		std::cout << "The result of 2 + 2 is: " << result.caption() << "\n";
		int r=std::stoi(result.caption());

		//char c; std::cin >> c;

		if ( r != 4 )
			exit(r?r:1);
		//assert(r != 4);
		//API::exit();
	}
#endif

	);
}
