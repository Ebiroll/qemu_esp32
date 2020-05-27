#include <iostream>
#include <nana/gui.hpp>
#include <nana/gui/timer.hpp>
#include "plot.h"
#include <cmath>

int main()
{

    using namespace nana;

    try
    {
        form fm;

        // construct plot to be drawn on form
        plot::plot thePlot( fm );

        // construct plot trace
        // displaying 100 points before they scroll off the plot
        plot::trace& t1 = thePlot.AddRealTimeTrace( 100 );

        // plot in blue
        t1.color( colors::blue );

        // create timer to provide new data regularly
        timer theTimer;
        theTimer.interval(std::chrono::milliseconds(10));
        theTimer.elapse([ &t1 ]()
        {
            static int p = 0;
            t1.add( 10 * sin( p++ / 10.0 ) );
        });
        theTimer.start();

        // show and run
        fm.show();
        exec();
    }
    catch( std::runtime_error& e )
    {
        msgbox mb( e.what() );
        mb();
    }
}

