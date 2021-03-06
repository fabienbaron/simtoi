 /*
 * This file is part of the SImulation and Modeling Tool for Optical
 * Interferometry (SIMTOI).
 *
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation version 3.
 *
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (c) 2015 Brian Kloppenborg
 */

#include "wParameterEditor.h"

#include <QMessageBox>

#include "CGLWidget.h"
#include "CTreeModel.h"
#include "CParameterItem.h"
#include "CModel.h"
#include "CModelList.h"
#include "CFeature.h"
#include "guiModelEditor.h"

wParameterEditor::wParameterEditor(QWidget * parent)
	: QWidget(parent)
{
	this->setupUi(this);

	mGLWidget = NULL;
}

wParameterEditor::~wParameterEditor()
{

}

/// Builds the tree model used to display the parameters in the GUI.
void wParameterEditor::buildTree()
{
	if(!mGLWidget)
		return;

	QStringList labels = QStringList();
	labels << "Name" << "Free" << "Value" << "Min" << "Max" << "Step";
	mTreeModel.clear();
	mTreeModel.setColumnCount(5);
	mTreeModel.setHorizontalHeaderLabels(labels);
	CModelListPtr model_list = mGLWidget->getModels();

	QList<QStandardItem *> items;
	QStandardItem * item;
	QStandardItem * item_parent;
	shared_ptr<CModel> model;
	shared_ptr<CPosition> position;
	CShader * shader;

	// Now pull out the pertinent information
	// NOTE: We drop the shared_ptrs here
	// TODO: Propigate shared pointers
	for(int i = 0; i < model_list->size(); i++)
	{
		// First pull out the model parameters
		model = model_list->GetModel(i);

		items = wParameterEditor::LoadParametersHeader(QString("Model"), model.get());
		item_parent = items[0];
		mTreeModel.appendRow(items);
		wParameterEditor::LoadParameters(item_parent, model.get());

		// Now for the Position Parameters
		position = model->GetPosition();
		items = wParameterEditor::LoadParametersHeader(QString("Position"), position.get());
		item = items[0];
		item_parent->appendRow(items);
		wParameterEditor::LoadParameters(item, position.get());

		// Lastly for the shader:
		shader = model->GetShader().get();
		items = wParameterEditor::LoadParametersHeader(QString("Shader"), shader);
		item = items[0];
		item_parent->appendRow(items);
		wParameterEditor::LoadParameters(item, shader);

		auto features = model->GetFeatures();
		for(auto feature: features)
		{
			items = wParameterEditor::LoadParametersHeader(QString("Feature"), feature.get());
			item = items[0];
			item_parent->appendRow(items);
			wParameterEditor::LoadParameters(item, feature.get());
		}
	}
}

/// Determines the index of the current selected model in the GUI.
int wParameterEditor::getSelectedModelIndex()
{
	// Get the current selected item index
	QModelIndex index = this->treeModels->currentIndex();
	// Because the user can click anywhere within the UI, we need to work our
	// way up the first items below the root. This will be the header row for
	// a given model
	while(index.parent() != this->treeModels->rootIndex())
		index = index.parent();

	// Use the row number as a proxy for the model ID.
	return index.row();
}

/// Parses the CParameterMap to create several rows corresponding to parameters
/// within a SIMTOI model.
void wParameterEditor::LoadParameters(QStandardItem * parent_widget, CParameterMap * param_map)
{
	// Get a reference to the map.
	const map<string, CParameter> parameter_map = param_map->getParameterMap();

	for(auto id_parameter: parameter_map)
	{
		const string parameter_id = id_parameter.first;
		const CParameter parameter = id_parameter.second;

		QList<QStandardItem *> items;
		QStandardItem * item;

		// First the name and the tooltip
		item = new QStandardItem(QString::fromStdString( parameter.getHumanName() ));
		item->setToolTip(QString::fromUtf8(parameter.getHelpText().c_str()));
		items << item;

		// Now the checkbox
		item = new CParameterItem(param_map, parameter_id);
		item->setEditable(true);
		item->setCheckable(true);
		if(parameter.isFree())
			item->setCheckState(Qt::Checked);
		else
			item->setCheckState(Qt::Unchecked);
		items << item;

		// The Value
		item = new CParameterItem(param_map, parameter_id);
		item->setEditable(true);
		item->setData(QVariant( parameter.getValue() ), Qt::DisplayRole);
		items << item;

		// Minimum parameter value
		item = new CParameterItem(param_map, parameter_id);
		item->setEditable(true);
		item->setData(QVariant( parameter.getMin() ), Qt::DisplayRole);
		items << item;

		// Maximum parameter value
		item = new CParameterItem(param_map, parameter_id);
		item->setEditable(true);
		item->setData(QVariant( parameter.getMax() ), Qt::DisplayRole);
		items << item;

		// Maximum step size
		item = new CParameterItem(param_map, parameter_id);
		item->setEditable(true);
		item->setData(QVariant( parameter.getStepSize()), Qt::DisplayRole);
		items << item;

		parent_widget->appendRow(items);
	}
}

/// Parses the CParameter map to create a header row for a SIMTOI model.
QList<QStandardItem *> wParameterEditor::LoadParametersHeader(QString name, CParameterMap * param_map)
{
	QList<QStandardItem *> items;
	QStandardItem * item;
	item = new QStandardItem(name);
	items << item;
	item = new QStandardItem(QString(""));
	items << item;
	item = new QStandardItem(QString::fromStdString(param_map->name()));
	items << item;

	return items;
}

/// Slot which instantiates a model editor for adding a new SIMTOI model.
void wParameterEditor::on_btnAddModel_clicked(void)
{
	if(!mGLWidget)
		return;

    int id = 0;
    int n_features;

    guiModelEditor tmp;
    tmp.show();

    if(tmp.exec())
    {
    	mGLWidget->addModel(tmp.getModel());
    }

    toggleButtons();
    refreshTree();
}

/// Deletes the selected model from the model list
void wParameterEditor::on_btnRemoveModel_clicked(void)
{
	if(!mGLWidget)
		return;

	int index = getSelectedModelIndex();
	if(index < 0)
	{
		// An error was thrown. Display a message to the user
		QMessageBox msgBox;
		msgBox.setWindowTitle("SIMTOI Error");
		msgBox.setText("Please select a model to delete.");
		msgBox.exec();

		return;
	}

	mGLWidget->removeModel(index);
	mGLWidget->Render();

	toggleButtons();
    refreshTree();
}

/// Opens up an editing dialog for the currently selected model.
void wParameterEditor::on_btnEditModel_clicked()
{
	if(!mGLWidget)
		return;

	int old_model_index = getSelectedModelIndex();
	if(old_model_index < 0)
	{
		// An error was thrown. Display a message to the user
		QMessageBox msgBox;
		msgBox.setWindowTitle("SIMTOI Error");
		msgBox.setText("Please select a model to edit.");
		msgBox.exec();

		return;
	}


	CModelPtr old_model = mGLWidget->getModel(old_model_index);

	guiModelEditor dialog(old_model);
	if(dialog.exec())
	{
		CModelPtr new_model = dialog.getModel();
		mGLWidget->replaceModel(old_model_index, new_model);
	}

	mGLWidget->Render();

	toggleButtons();
    refreshTree();
}

/// Sets the current widget. Connects necessary signals and slots.
void wParameterEditor::setGLWidget(CGLWidget * gl_widget)
{
	mGLWidget = gl_widget;

	// connect any non-automatic signal/slots
	connect(&mTreeModel, SIGNAL(parameterUpdated()), mGLWidget, SLOT(updateParameters()));
	connect(mGLWidget, SIGNAL(modelUpdated()), this, SLOT(updateModels()));

	refreshTree();
}

/// Automatically activate/deactivate the buttons based upon the status
/// of elements in the GUI.
void wParameterEditor::toggleButtons()
{
	if(!mGLWidget)
		return;

	this->btnAddModel->setEnabled(false);
	this->btnEditModel->setEnabled(false);
	this->btnRemoveModel->setEnabled(false);

	// Buttons for add/edit/delete model
	this->btnAddModel->setEnabled(true);
	if(mTreeModel.rowCount() > 0)
	{
		this->btnEditModel->setEnabled(true);
		this->btnRemoveModel->setEnabled(true);
	}
}

/// Refreshes the tree with newly generated data.
void wParameterEditor::refreshTree()
{
	this->treeModels->setHeaderHidden(false);
	this->treeModels->setModel(&mTreeModel);
	this->treeModels->header()->setResizeMode(QHeaderView::ResizeToContents);
	// expand the tree fully
	this->treeModels->expandAll();
}

/// Slot which receives notifications when a SIMTOI model is updated elsewhere
/// within this software.
void wParameterEditor::updateModels()
{
	buildTree();
	refreshTree();
}
