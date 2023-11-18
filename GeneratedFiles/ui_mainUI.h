/********************************************************************************
** Form generated from reading UI file 'mainUI.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINUI_H
#define UI_MAINUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "medialistview.h"
#include "multicompleterlineedit.h"

QT_BEGIN_NAMESPACE

class Ui_mainUIClass
{
public:
    QWidget *centralWidget;
    QGridLayout *window_grid_layout;
    MultiCompleterLineEdit *query_line_edit;
    QPushButton *query_push_button;
    QGroupBox *tags_group_box;
    QGridLayout *gridLayout;
    QPushButton *new_tag_push_button;
    QPushButton *delete_tag_push_button;
    QTableView *tag_table_view;
    QLineEdit *tag_search_line_edit;
    QPushButton *all_media_push_button;
    QPushButton *tagless_media_push_button;
    QPushButton *clear_media_push_button;
    MediaListView *media_list_view;
    QGroupBox *media_info_group_box;
    QGridLayout *gridLayout_2;
    QGroupBox *media_info_info_groupbox;
    QFormLayout *formLayout;
    QLabel *media_info_id_title_label;
    QLabel *media_info_id_label;
    QLabel *media_info_name_title_label;
    QLabel *media_subdir_title_label;
    QLineEdit *media_info_name_line_edit;
    QLineEdit *media_info_subdir_line_edit;
    QLabel *media_hash_title_label;
    QLineEdit *media_info_hash_line_edit;
    QPushButton *media_info_hash_regen_push_button;
    MultiCompleterLineEdit *media_tag_search_line_edit;
    QPushButton *add_media_tag_push_button;
    QListView *media_tag_list_view;
    QLabel *media_info_thumb_label;
    QDockWidget *log_docked_widget;
    QWidget *dockWidgetContents_2;
    QVBoxLayout *verticalLayout;
    QListView *log_list_view;

    void setupUi(QMainWindow *mainUIClass)
    {
        if (mainUIClass->objectName().isEmpty())
            mainUIClass->setObjectName(QString::fromUtf8("mainUIClass"));
        mainUIClass->resize(1382, 750);
        centralWidget = new QWidget(mainUIClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        window_grid_layout = new QGridLayout(centralWidget);
        window_grid_layout->setSpacing(6);
        window_grid_layout->setContentsMargins(11, 11, 11, 11);
        window_grid_layout->setObjectName(QString::fromUtf8("window_grid_layout"));
        query_line_edit = new MultiCompleterLineEdit(centralWidget);
        query_line_edit->setObjectName(QString::fromUtf8("query_line_edit"));
        query_line_edit->setEnabled(false);

        window_grid_layout->addWidget(query_line_edit, 1, 3, 1, 1);

        query_push_button = new QPushButton(centralWidget);
        query_push_button->setObjectName(QString::fromUtf8("query_push_button"));
        query_push_button->setEnabled(false);

        window_grid_layout->addWidget(query_push_button, 1, 4, 1, 1);

        tags_group_box = new QGroupBox(centralWidget);
        tags_group_box->setObjectName(QString::fromUtf8("tags_group_box"));
        gridLayout = new QGridLayout(tags_group_box);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        new_tag_push_button = new QPushButton(tags_group_box);
        new_tag_push_button->setObjectName(QString::fromUtf8("new_tag_push_button"));
        new_tag_push_button->setEnabled(false);

        gridLayout->addWidget(new_tag_push_button, 2, 0, 1, 1);

        delete_tag_push_button = new QPushButton(tags_group_box);
        delete_tag_push_button->setObjectName(QString::fromUtf8("delete_tag_push_button"));
        delete_tag_push_button->setEnabled(false);

        gridLayout->addWidget(delete_tag_push_button, 2, 1, 1, 1);

        tag_table_view = new QTableView(tags_group_box);
        tag_table_view->setObjectName(QString::fromUtf8("tag_table_view"));
        tag_table_view->setSortingEnabled(true);

        gridLayout->addWidget(tag_table_view, 0, 0, 1, 2);

        tag_search_line_edit = new QLineEdit(tags_group_box);
        tag_search_line_edit->setObjectName(QString::fromUtf8("tag_search_line_edit"));
        tag_search_line_edit->setEnabled(false);

        gridLayout->addWidget(tag_search_line_edit, 1, 0, 1, 2);


        window_grid_layout->addWidget(tags_group_box, 1, 6, 5, 1);

        all_media_push_button = new QPushButton(centralWidget);
        all_media_push_button->setObjectName(QString::fromUtf8("all_media_push_button"));
        all_media_push_button->setEnabled(false);
        all_media_push_button->setCursor(QCursor(Qt::ArrowCursor));

        window_grid_layout->addWidget(all_media_push_button, 1, 1, 1, 1);

        tagless_media_push_button = new QPushButton(centralWidget);
        tagless_media_push_button->setObjectName(QString::fromUtf8("tagless_media_push_button"));
        tagless_media_push_button->setEnabled(false);

        window_grid_layout->addWidget(tagless_media_push_button, 1, 2, 1, 1);

        clear_media_push_button = new QPushButton(centralWidget);
        clear_media_push_button->setObjectName(QString::fromUtf8("clear_media_push_button"));
        clear_media_push_button->setEnabled(false);

        window_grid_layout->addWidget(clear_media_push_button, 1, 0, 1, 1);

        media_list_view = new MediaListView(centralWidget);
        media_list_view->setObjectName(QString::fromUtf8("media_list_view"));

        window_grid_layout->addWidget(media_list_view, 2, 3, 4, 2);

        media_info_group_box = new QGroupBox(centralWidget);
        media_info_group_box->setObjectName(QString::fromUtf8("media_info_group_box"));
        gridLayout_2 = new QGridLayout(media_info_group_box);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        media_info_info_groupbox = new QGroupBox(media_info_group_box);
        media_info_info_groupbox->setObjectName(QString::fromUtf8("media_info_info_groupbox"));
        formLayout = new QFormLayout(media_info_info_groupbox);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        media_info_id_title_label = new QLabel(media_info_info_groupbox);
        media_info_id_title_label->setObjectName(QString::fromUtf8("media_info_id_title_label"));

        formLayout->setWidget(1, QFormLayout::LabelRole, media_info_id_title_label);

        media_info_id_label = new QLabel(media_info_info_groupbox);
        media_info_id_label->setObjectName(QString::fromUtf8("media_info_id_label"));

        formLayout->setWidget(1, QFormLayout::FieldRole, media_info_id_label);

        media_info_name_title_label = new QLabel(media_info_info_groupbox);
        media_info_name_title_label->setObjectName(QString::fromUtf8("media_info_name_title_label"));

        formLayout->setWidget(2, QFormLayout::LabelRole, media_info_name_title_label);

        media_subdir_title_label = new QLabel(media_info_info_groupbox);
        media_subdir_title_label->setObjectName(QString::fromUtf8("media_subdir_title_label"));

        formLayout->setWidget(3, QFormLayout::LabelRole, media_subdir_title_label);

        media_info_name_line_edit = new QLineEdit(media_info_info_groupbox);
        media_info_name_line_edit->setObjectName(QString::fromUtf8("media_info_name_line_edit"));

        formLayout->setWidget(2, QFormLayout::FieldRole, media_info_name_line_edit);

        media_info_subdir_line_edit = new QLineEdit(media_info_info_groupbox);
        media_info_subdir_line_edit->setObjectName(QString::fromUtf8("media_info_subdir_line_edit"));

        formLayout->setWidget(3, QFormLayout::FieldRole, media_info_subdir_line_edit);

        media_hash_title_label = new QLabel(media_info_info_groupbox);
        media_hash_title_label->setObjectName(QString::fromUtf8("media_hash_title_label"));

        formLayout->setWidget(4, QFormLayout::LabelRole, media_hash_title_label);

        media_info_hash_line_edit = new QLineEdit(media_info_info_groupbox);
        media_info_hash_line_edit->setObjectName(QString::fromUtf8("media_info_hash_line_edit"));

        formLayout->setWidget(4, QFormLayout::FieldRole, media_info_hash_line_edit);

        media_info_hash_regen_push_button = new QPushButton(media_info_info_groupbox);
        media_info_hash_regen_push_button->setObjectName(QString::fromUtf8("media_info_hash_regen_push_button"));
        media_info_hash_regen_push_button->setEnabled(false);

        formLayout->setWidget(5, QFormLayout::SpanningRole, media_info_hash_regen_push_button);


        gridLayout_2->addWidget(media_info_info_groupbox, 1, 0, 1, 2);

        media_tag_search_line_edit = new MultiCompleterLineEdit(media_info_group_box);
        media_tag_search_line_edit->setObjectName(QString::fromUtf8("media_tag_search_line_edit"));
        media_tag_search_line_edit->setEnabled(false);

        gridLayout_2->addWidget(media_tag_search_line_edit, 6, 0, 1, 1);

        add_media_tag_push_button = new QPushButton(media_info_group_box);
        add_media_tag_push_button->setObjectName(QString::fromUtf8("add_media_tag_push_button"));
        add_media_tag_push_button->setEnabled(false);

        gridLayout_2->addWidget(add_media_tag_push_button, 6, 1, 1, 1);

        media_tag_list_view = new QListView(media_info_group_box);
        media_tag_list_view->setObjectName(QString::fromUtf8("media_tag_list_view"));

        gridLayout_2->addWidget(media_tag_list_view, 2, 0, 1, 2);

        media_info_thumb_label = new QLabel(media_info_group_box);
        media_info_thumb_label->setObjectName(QString::fromUtf8("media_info_thumb_label"));

        gridLayout_2->addWidget(media_info_thumb_label, 0, 0, 1, 2);


        window_grid_layout->addWidget(media_info_group_box, 2, 0, 4, 3);

        mainUIClass->setCentralWidget(centralWidget);
        log_docked_widget = new QDockWidget(mainUIClass);
        log_docked_widget->setObjectName(QString::fromUtf8("log_docked_widget"));
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName(QString::fromUtf8("dockWidgetContents_2"));
        verticalLayout = new QVBoxLayout(dockWidgetContents_2);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        log_list_view = new QListView(dockWidgetContents_2);
        log_list_view->setObjectName(QString::fromUtf8("log_list_view"));
        log_list_view->setMaximumSize(QSize(16777215, 16777215));

        verticalLayout->addWidget(log_list_view);

        log_docked_widget->setWidget(dockWidgetContents_2);
        mainUIClass->addDockWidget(static_cast<Qt::DockWidgetArea>(8), log_docked_widget);

        retranslateUi(mainUIClass);

        QMetaObject::connectSlotsByName(mainUIClass);
    } // setupUi

    void retranslateUi(QMainWindow *mainUIClass)
    {
        mainUIClass->setWindowTitle(QApplication::translate("mainUIClass", "mainUI", nullptr));
        query_push_button->setText(QApplication::translate("mainUIClass", "Query", nullptr));
        tags_group_box->setTitle(QApplication::translate("mainUIClass", "Tags", nullptr));
        new_tag_push_button->setText(QApplication::translate("mainUIClass", "Create Tag", nullptr));
        delete_tag_push_button->setText(QApplication::translate("mainUIClass", "Delete tag", nullptr));
        all_media_push_button->setText(QApplication::translate("mainUIClass", "All media", nullptr));
        tagless_media_push_button->setText(QApplication::translate("mainUIClass", "NoTag Media", nullptr));
        clear_media_push_button->setText(QApplication::translate("mainUIClass", "Clear media", nullptr));
        media_info_group_box->setTitle(QApplication::translate("mainUIClass", "Media Info", nullptr));
        media_info_info_groupbox->setTitle(QString());
        media_info_id_title_label->setText(QApplication::translate("mainUIClass", "ID:", nullptr));
        media_info_id_label->setText(QString());
        media_info_name_title_label->setText(QApplication::translate("mainUIClass", "Name:", nullptr));
        media_subdir_title_label->setText(QApplication::translate("mainUIClass", "Subdir:", nullptr));
        media_hash_title_label->setText(QApplication::translate("mainUIClass", "Hash:", nullptr));
        media_info_hash_regen_push_button->setText(QApplication::translate("mainUIClass", "Regenerate Hash", nullptr));
        add_media_tag_push_button->setText(QApplication::translate("mainUIClass", "Add", nullptr));
        media_info_thumb_label->setText(QString());
        log_docked_widget->setWindowTitle(QApplication::translate("mainUIClass", "Log", nullptr));
    } // retranslateUi

};

namespace Ui {
    class mainUIClass: public Ui_mainUIClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINUI_H
