#include "ut_settingsdialog.h"

test_settingsdialog::test_settingsdialog()
{

}


extern void GenerateSettingTranslate();
TEST_F(test_settingsdialog, GenerateSettingTranslate)
{

    GenerateSettingTranslate();
    
}
