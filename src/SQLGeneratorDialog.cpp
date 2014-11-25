#include "SQLGeneratorDialog.h"




SQLGeneratorDialog::SQLGeneratorDialog(QWidget* parent)
		: QDialog(parent)
{
	ui.setupUi(this);

	ui.dbTypeBox->addItem(tr("SQLite"), DatabaseConnection::SQLite);

#ifdef EDB_MYSQL_ENABLED
	ui.dbTypeBox->addItem(tr("MySQL"), DatabaseConnection::MySQL);
#endif

	connect(ui.genCodeButton, SIGNAL(clicked()), this, SLOT(genCodeButtonClicked()));
}


void SQLGeneratorDialog::genCodeButtonClicked()
{
	if (ui.codeTypeTabber->currentWidget() == ui.pcatWidget) {
		QString code = QString(
				"CREATE TABLE pcatmeta_%1 (\n"
				"	id VARCHAR(64) PRIMARY KEY  NOT NULL,\n"
				"	\n"
				"	name VARCHAR(128),\n"
				"	type VARCHAR(32) DEFAULT 'string',\n"
				"	\n"
				"	unit_suffix VARCHAR(32),\n"
				"	display_unit_affix VARCHAR(16),\n"
				"	\n"
				"	int_range_min INTEGER,\n"
				"	int_range_max INTEGER,\n"
				"	\n"
				"	decimal_range_min DOUBLE,\n"
				"	decimal_range_max DOUBLE,\n"
				"	\n"
				"	string_max_length INTEGER,\n"
				"	\n"
				"	pl_category VARCHAR(64),\n"
				"	\n"
				"	flag_full_text_indexed BOOLEAN,\n"
				"	flag_multi_value BOOLEAN,\n"
				"	flag_si_prefixes_default_to_base2 BOOLEAN,\n"
				"	flag_display_with_units BOOLEAN,\n"
				"	flag_display_no_selection_list BOOLEAN,\n"
				"	flag_display_hide_from_listing_table BOOLEAN,\n"
				"	flag_display_text_area BOOLEAN,\n"
				"	flag_display_multi_in_single_field BOOLEAN,\n"
				"	flag_display_dynamic_enum BOOLEAN,\n"
				"   \n"
				"	bool_true_text VARCHAR(128),\n"
				"	bool_false_text VARCHAR(128),\n"
				"	\n"
				"	ft_user_prefix VARCHAR(64),\n"
				"	\n"
				"	textarea_min_height INTEGER,\n"
				"	\n"
				"	sql_natural_order_code VARCHAR(512),\n"
				"	sql_ascending_order_code VARCHAR(512),\n"
				"	sql_descending_order_code VARCHAR(512),\n"
				"   \n"
				"   sortidx INTEGER DEFAULT 10000\n"
				");\n"
				"\n"
				"INSERT INTO part_category (id, name, desc_pattern) VALUES ('%1', '%2', '%3');\n"
			).arg(ui.pcatIDField->text()).arg(ui.pcatUserReadableNameField->text()).arg(ui.pcatDescPatternField->text());

		ui.codeArea->setPlainText(code);
	} else {
		ui.codeArea->clear();
	}
}
