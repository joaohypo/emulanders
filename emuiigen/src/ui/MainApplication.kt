package com.xortroll.emuiibo.emuiigen.ui

import java.util.Properties
import javafx.application.Application
import javafx.application.HostServices
import javafx.fxml.FXMLLoader
import javafx.geometry.Insets
import javafx.geometry.Pos
import javafx.scene.Parent
import javafx.scene.Scene
import javafx.scene.control.Label
import javafx.scene.layout.Background
import javafx.scene.layout.BackgroundFill
import javafx.scene.layout.CornerRadii
import javafx.scene.layout.HBox
import javafx.scene.layout.Pane
import javafx.scene.image.Image
import javafx.scene.paint.Color
import javafx.scene.text.Font.font
import javafx.stage.Stage

class MainApplication : Application() {
    private lateinit var controller: MainController;

    companion object {

        val Description = "emuiibo's virtual amiibo PC utility";
        lateinit var Version: String;

        lateinit var HostServices: HostServices;

        fun main(args: Array<String>) {
            Application.launch(MainApplication::class.java, *args);
        }

    }

    override fun start(stage: Stage) {
        HostServices = this.getHostServices();

        val this_loader = this::class.java.classLoader;
        val fxml_loader = FXMLLoader(this_loader.getResource("main.fxml"));
        val base = fxml_loader.load<Pane>();
        this.controller = fxml_loader.getController() as MainController;

        val main_props = Properties();
        main_props.load(this_loader.getResourceAsStream("main.properties"));
        Version = main_props.getProperty("version");
        
        val width = base.getPrefWidth();
        val height = base.getPrefHeight();
        stage.title = "emuiigen v" + Version + " - " + Description;
        stage.icons.add(Image(this_loader.getResource("icon.png").toExternalForm()));
        stage.scene = Scene(base, width, height);
        stage.scene.stylesheets.add(this_loader.getResource("main.css").toExternalForm());
        stage.minWidth = width;
        stage.minHeight = height;
        stage.setResizable(false);

        this.controller.prepare(stage);
        stage.show();
    }

}