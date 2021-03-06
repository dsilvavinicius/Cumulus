#ifndef TEXT_TEST_WIDGET
#define TEXT_TEST_WIDGET

#include <utils/qtplainwidget.hpp>
#include "omicron/renderer/text_effect.h"

namespace omicron::test::ui
{
    using namespace omicron::renderer;
    
    class TextTestWidget
    : public QtPlainWidget
    {
        Q_OBJECT
        
    public:
        TextTestWidget( QWidget *parent = 0 );
        
        void initializeGL();
        
        void paintGL();
        
    private:
        TextEffect m_textEffect;
    };
}

#endif
