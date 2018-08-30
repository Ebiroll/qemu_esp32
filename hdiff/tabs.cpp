#include <iostream>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/tabbar.hpp>

int main()
{
    nana::form  fm{
        nana::API::make_center(320, 300),
        nana::appear::decorate<nana::appear::sizable>()};

    nana::panel<false>  page0{fm};
    nana::place         place{page0};
    nana::listbox       lbox{page0};
    place.div("<margin=5 lbox>");
    place["lbox"] <<lbox;
    lbox.append_header("item-1", 60);
    lbox.append_header("item-2", 60);
    lbox.append_header("item-3", 60);
    lbox.enable_single(true, false);
    page0.events().expose([&](const nana::arg_expose &arg){
        std::cout <<"Page Export exposed=" <<arg.exposed <<'\n';
        if (arg.exposed)
        {
            lbox.avoid_drawing([&]{
                lbox.clear();
                auto rootcat = lbox.at(0);
                for (int i = 0; i < 30; ++i)
                {
                    const auto s = std::to_string(i);
                    rootcat.append({s, s, s});
                }
                std::cout <<"Total " <<rootcat.size() <<" rows added\n";
            });
            for (auto &i: lbox.at(0))
                if (i.text(0) == "22")
                {
                    i.select(true, true); // Causes the all rows invisible
                    //i.select(true);
                    std::cout <<"22 selected\n";
                    break;
                }
        }
    });

    nana::button btn{fm, "Select 28"};
    btn.events().click([&]{
        for (auto &i: lbox.at(0))
            if (i.text(0) == "28")
            {
                i.select(true, true); // Causes the all rows invisible
                //i.select(true);
                std::cout <<"28 selected\n";
                break;
            }
    });

    fm.div("vert "
           "<weight=25 tabs>"
           "<pages>"
           "<marginr=10 weight=30 btn>");

    nana::panel<false>  page1{fm};
    nana::panel<false>  page2{fm};
    fm["pages"].fasten(page0)
               .fasten(page1)
               .fasten(page2);
    nana::tabbar<bool>  tabs{fm};
    fm["tabs"] <<tabs;
    tabs.append("Page 0", page0);
    tabs.append("Page 1", page1);
    tabs.append("Page 2", page2);
    tabs.activated(0);
    fm["btn"]   << btn;
    fm.collocate();
    fm.show();
    nana::exec();
}
