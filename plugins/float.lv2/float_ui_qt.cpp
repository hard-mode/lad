/* Float.lv2 Qt GUI
 * Copyright 2011-2011 David Robillard <http://drobilla.net/>
 *
 * Float.lv2 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Float.lv2 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include "QtGui"

#define FLOAT_UI_QT_URI "http://drobilla.net/plugins/float-ui-qt"

extern "C" {

static LV2UI_Descriptor* ui_descriptor = NULL;

typedef struct {
	LV2UI_Write_Function write_function;
	LV2UI_Controller     controller;
	QDoubleSpinBox*      spin;
} FloatUiQt;

static LV2UI_Handle
float_ui_qt_instantiate(const struct _LV2UI_Descriptor* descriptor,
                        const char*                     plugin_uri,
                        const char*                     bundle_path,
                        LV2UI_Write_Function            write_function,
                        LV2UI_Controller                controller,
                        LV2UI_Widget*                   widget,
                        const LV2_Feature* const*       features)
{
	FloatUiQt* me = (FloatUiQt*)malloc(sizeof(FloatUiQt));

	me->write_function = write_function;
	me->controller     = controller;

	me->spin = new QDoubleSpinBox();
	me->spin->setRange(-1.0, 1.0);
	me->spin->setSingleStep(0.1);
	me->spin->setValue(0.0);

	*widget = me->spin;

	return me;
}

static void
float_ui_qt_cleanup(LV2UI_Handle ui)
{
	FloatUiQt* me = (FloatUiQt*)ui;
	delete me->spin;
	me->spin = NULL;
}

static void
float_ui_qt_port_event(LV2UI_Handle ui,
                       uint32_t     port_index,
                       uint32_t     buffer_size,
                       uint32_t     format,
                       const void*  buffer)
{
}

static void
init()
{
	ui_descriptor = (LV2UI_Descriptor*)malloc(sizeof(LV2UI_Descriptor));
	ui_descriptor->URI            = FLOAT_UI_QT_URI;
	ui_descriptor->instantiate    = float_ui_qt_instantiate;
	ui_descriptor->cleanup        = float_ui_qt_cleanup;
	ui_descriptor->port_event     = float_ui_qt_port_event;
	ui_descriptor->extension_data = NULL;
}

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	if (!ui_descriptor) {
		init();
	}

	switch (index) {
	case 0:
		return ui_descriptor;
	default:
		return NULL;
	}
}

} // extern "C"
