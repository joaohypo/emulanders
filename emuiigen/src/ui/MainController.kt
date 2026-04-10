package com.xortroll.emuiibo.emuiigen.ui

import java.nio.file.Paths
import java.time.LocalDateTime
import java.util.concurrent.Callable
import java.io.File
import java.io.FileInputStream
import javafx.fxml.FXML
import javafx.scene.control.Button
import javafx.scene.control.ComboBox
import javafx.scene.control.CheckBox
import javafx.scene.control.Label
import javafx.scene.control.TextField
import javafx.scene.control.ListView
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.image.ImageView
import javafx.scene.image.Image
import javafx.event.EventHandler
import javafx.event.ActionEvent
import javafx.stage.Stage
import javafx.stage.DirectoryChooser
import javafx.application.Platform
import javafx.beans.value.ObservableValue
import javafx.beans.value.ChangeListener
import javafx.beans.binding.Bindings
import javafx.concurrent.Task
import javafx.application.HostServices
import org.apache.commons.io.FileUtils
import org.apache.commons.net.ftp.FTPClient
import org.apache.commons.net.ftp.FTPClientConfig
import org.apache.commons.net.ftp.FTPReply
import com.xortroll.emuiibo.emuiigen.Amiibo
import com.xortroll.emuiibo.emuiigen.AmiiboStatus
import com.xortroll.emuiibo.emuiigen.AmiiboStatusKind
import com.xortroll.emuiibo.emuiigen.AmiiboAPI
import com.xortroll.emuiibo.emuiigen.AmiiboAPIEntry
import com.xortroll.emuiibo.emuiigen.Utils
import com.xortroll.emuiibo.emuiigen.AmiiboDate
import com.xortroll.emuiibo.emuiigen.ui.MainApplication
import com.xortroll.emuiibo.emuiigen.AmiiboAreaInfo

class MainController {
    companion object {

        val TemporaryFtpDirectory = "tmp_ftp";

    }

    @FXML lateinit var AmiiboOpenButton: Button;
    @FXML lateinit var AmiiboSaveButton: Button;
    @FXML lateinit var OpenedAmiiboNameText: TextField;
    @FXML lateinit var OpenedAmiiboUseRandomUuidCheck: CheckBox;
    @FXML lateinit var OpenedAmiiboAreaList: ListView<String>;

    @FXML lateinit var AboutButton: Button;
    
    @FXML lateinit var GenerateOneAmiiboSeriesBox: ComboBox<String>;
    @FXML lateinit var AmiiboBox: ComboBox<String>;
    @FXML lateinit var AmiiboImage: ImageView;

    @FXML lateinit var ApiUrlText: TextField;
    @FXML lateinit var ApiReloadButton: Button;
    @FXML lateinit var StatusLabel: Label;

    @FXML lateinit var AmiiboNameText: TextField;
    @FXML lateinit var AmiiboDirectoryText: TextField;
    @FXML lateinit var NameAsDirectoryNameCheck: CheckBox;
    @FXML lateinit var GenerateOneUseRandomUuidCheck: CheckBox;
    @FXML lateinit var GenerateOneImageSaveCheck: CheckBox;
    @FXML lateinit var GenerateOneFtpCheck: CheckBox;
    @FXML lateinit var GenerateOneFtpAddress: TextField;
    @FXML lateinit var GenerateOneFtpPort: TextField;
    @FXML lateinit var GenerateOneButton: Button;

    @FXML lateinit var GenerateAllUseRandomUuidCheck: CheckBox;
    @FXML lateinit var GenerateAllImageSaveCheck: CheckBox;
    @FXML lateinit var GenerateAllFtpCheck: CheckBox;
    @FXML lateinit var GenerateAllFtpAddress: TextField;
    @FXML lateinit var GenerateAllFtpPort: TextField;
    @FXML lateinit var GenerateAllButton: Button;

    @FXML lateinit var GenerateSeriesUseRandomUuidCheck: CheckBox;
    @FXML lateinit var GenerateSeriesImageSaveCheck: CheckBox;
    @FXML lateinit var GenerateSeriesAmiiboSeriesBox: ComboBox<String>;
    @FXML lateinit var GenerateSeriesFtpCheck: CheckBox;
    @FXML lateinit var GenerateSeriesFtpAddress: TextField;
    @FXML lateinit var GenerateSeriesFtpPort: TextField;
    @FXML lateinit var GenerateSeriesButton: Button;

    lateinit var MainStage: Stage;
    lateinit var OpenedAmiiboPath: String;
    var OpenedAmiibo: Amiibo? = null;
    lateinit var Amiibos: Map<String, List<AmiiboAPIEntry>>;

    fun updateSelectedAmiiboSeries() {
        val series = this.getSelectedAmiiboSeriesName();
        if(series != null) {
            val amiibos = this.Amiibos.get(series);
            amiibos?.let {
                val amiibo_names = mutableListOf<String>();
                for(amiibo in amiibos) {
                    amiibo_names.add(amiibo.amiibo_name);
                }
                amiibo_names.sort();
                this.AmiiboBox.items.setAll(amiibo_names);
            }
            ?: let {
                System.out.println("Internal unexpected error");
            }
        }
    }

    inline fun getSelectedAmiiboSeriesName() : String? {
        return this.GenerateOneAmiiboSeriesBox.selectionModel.selectedItem;
    }

    inline fun getSelectedAmiiboIndex() : Int {
        return this.AmiiboBox.selectionModel.selectedIndex;
    }
    
    inline fun getSelectedAmiibo() : AmiiboAPIEntry? {
        val series_name = this.getSelectedAmiiboSeriesName();
        if(series_name != null) {
            return this.Amiibos.get(series_name)?.get(this.getSelectedAmiiboIndex());
        }
        else {
            return null;
        }
    }

    fun updateSelectedAmiibo() {
        val series = this.getSelectedAmiiboSeriesName();
        if(series != null) {
            val amiibos = this.Amiibos.get(series);
            amiibos?.let {
                val amiibo_idx = this.getSelectedAmiiboIndex();

                if(amiibo_idx >= 0) {
                    val amiibo = amiibos.get(amiibo_idx);

                    val img = Image(amiibo.image_url, true);
                    this@MainController.AmiiboImage.setImage(img);
                    this@MainController.AmiiboNameText.setText(amiibo.amiibo_name);
                }
            }
            ?: let {
                this@MainController.showError("Internal unexpected error...");
            }
        }
        else {
            this@MainController.AmiiboImage.setImage(null);
        }
    }

    fun showYesNo(msg: String) : Boolean {
        val alert = Alert(Alert.AlertType.CONFIRMATION, msg, ButtonType.YES, ButtonType.CANCEL);
        val res = alert.showAndWait();
        return res.isPresent() && (res.get() == ButtonType.YES);
    }
    
    fun showError(msg: String) {
        val alert = Alert(Alert.AlertType.ERROR, msg, ButtonType.OK);
        alert.showAndWait();
    }

    fun showWarn(msg: String) {
        val alert = Alert(Alert.AlertType.WARNING, msg, ButtonType.OK);
        alert.showAndWait();
    }

    fun showInfo(msg: String) {
        val alert = Alert(Alert.AlertType.INFORMATION, msg, ButtonType.OK);
        alert.showAndWait();
    }

    fun cleanTemporaryFtp() {
        FileUtils.deleteDirectory(Paths.get(TemporaryFtpDirectory).toAbsolutePath().toFile());
    }

    fun processFtpPort(is_ftp: Boolean, port_str: String) : Int? {
        if(is_ftp) {
            try {
                return port_str.toInt();
            }
            catch(ex: Exception) {
                this.showError("Invalid FTP port: " + ex.toString());
                return null;
            }
        }
        else {
            return 0;
        }
    }

    fun chooseBaseAmiiboPath(is_ftp: Boolean) : Pair<String, String>? {
        val path = if(is_ftp) {
            Paths.get(TemporaryFtpDirectory).toAbsolutePath()
        }
        else {
            val chooser = DirectoryChooser();
            val dir = chooser.showDialog(this@MainController.MainStage);
            if(dir != null) {
                Paths.get(dir.toString()).toAbsolutePath()
            }
            else {
                null
            }
        };

        val pres_path = if(is_ftp) {
            "ftp:/emuiibo/amiibo"
        }
        else {
            path.toString()
        };

        return if(path != null) {
            Pair(path.toString(), pres_path)
        }
        else {
            null
        };
    }

    fun generateAmiibo(path: String, base_path: String, amiibo: AmiiboAPIEntry, amiibo_name: String, use_random_uuid: Boolean, save_image: Boolean, is_ftp: Boolean, ftp_addr: String, ftp_port: Int) : Boolean {
        val local_date = LocalDateTime.now();
        val cur_date = AmiiboDate(local_date.year.toUShort(), local_date.monthValue.toUByte(), local_date.dayOfMonth.toUByte());

        val write_counter: UShort = 0u;
        val version: UInt = 0u;
        val mii_charinfo_file = "mii-charinfo.bin";
        val uuid = Amiibo.randomUuid();

        val amiibo_v = Amiibo(cur_date, amiibo.id, cur_date, mii_charinfo_file, amiibo_name, uuid, use_random_uuid, version, write_counter, null);
        if(amiibo_v.save(path)) {
            if(save_image) {
                try {
                    val image_path = Paths.get(path, "amiibo.png").toAbsolutePath().toString();
                    Utils.netDownloadFile(amiibo.image_url, image_path);
                }
                catch(ex: Exception) {
                    System.out.println("Exception saving amiibo image: " + ex.toString());
                    return false;
                }
            }

            if(is_ftp) {
                val ftp_base_dir = "/emuiibo/amiibo/" + base_path;
                val client = FTPClient();

                try {
                    val config = FTPClientConfig();
                    client.configure(config);
                    client.connect(ftp_addr, ftp_port);

                    val reply = client.replyCode;
                    if(!FTPReply.isPositiveCompletion(reply)) {
                        this@MainController.showError("The FTP server refused connection...");
                        client.disconnect();
                        return false;
                    }

                    client.makeDirectory(ftp_base_dir);
                    if(!Utils.ensureFtpDirectory(client, ftp_base_dir)) {
                        this@MainController.showError("Unable to ensure FTP directory '" + ftp_base_dir + "': " + client.replyCode.toString());
                        client.disconnect();
                        return false;
                    }

                    client.enterLocalPassiveMode();
                    for(file in File(path).listFiles()) {
                        val strm = FileInputStream(file);
                        val ftp_path = ftp_base_dir + "/" + file.name;
                        client.storeFile(ftp_path, strm);
                        strm.close();
                        if(!FTPReply.isPositiveCompletion(client.replyCode)) {
                            this@MainController.showError("Unable to store FTP file '" + ftp_path + "': " + client.replyCode.toString());
                            client.disconnect();
                            return false;
                        }
                    }

                    client.disconnect();
                    return true;
                }
                catch(ex: Exception) {
                    this@MainController.showError("Exception on FTP connection: " + ex.toString());
                    client.disconnect();
                    return false;
                }
            }

            return true;
        }
        else {
            return false;
        }
    }

    private fun tryReadApi(url: String) {
        val task = object : Task<Unit>() {
            override fun call() {
                this.updateMessage("Accessing AmiiboAPI...");

                this@MainController.ApiUrlText.setDisable(true);
                this@MainController.ApiReloadButton.setDisable(true);
                val api_amiibos = AmiiboAPI.readApi(url);
                this@MainController.ApiUrlText.setDisable(false);
                this@MainController.ApiReloadButton.setDisable(false);

                api_amiibos?.let {
                    this.updateMessage("AmiiboAPI was successfully accessed!");
                    
                    Platform.runLater {
                        this@MainController.Amiibos = api_amiibos;

                        val series_names = api_amiibos.keys.toMutableList();
                        series_names.sort();
                        this@MainController.GenerateOneAmiiboSeriesBox.items.setAll(series_names);
                        this@MainController.GenerateOneAmiiboSeriesBox.selectionModel.select(0);
                        this@MainController.GenerateSeriesAmiiboSeriesBox.items.setAll(series_names);
                        this@MainController.GenerateSeriesAmiiboSeriesBox.selectionModel.select(0);
                    }
                }
                ?: let {
                    this.updateMessage("AmiiboAPI could not be accessed...");
                }
            }
        };

        this.StatusLabel.textProperty().bind(task.messageProperty());

        val task_thr = Thread(task);
        task_thr.setDaemon(true);
        task_thr.start();
    }

    fun prepare(stage: Stage) {
        this.MainStage = stage;
        this.OpenedAmiibo = null;

        this.tryReadApi(AmiiboAPI.DefaultUrl);

        this.AmiiboOpenButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                val chooser = DirectoryChooser();
                val dir = chooser.showDialog(this@MainController.MainStage);
                if(dir != null) {
                    val path = Paths.get(dir.toString()).toAbsolutePath().toString();

                    val (status, amiibo) = Amiibo.tryParse(path);
                    if(amiibo == null) {
                        this@MainController.showError("Unable to load amiibo:\n" + status.toString());
                    }
                    else {
                        if(status.contains(AmiiboStatusKind.MiiCharInfoNotFound)) {
                            this@MainController.showWarn("This amiibo has no mii charinfo file.\nIt is still a valid amiibo, emuiibo will generate it on boot.");
                        }
                        if(status.contains(AmiiboStatusKind.InvalidNameLength)) {
                            this@MainController.showWarn("This amiibo had a name that exceeds 10 characters.\nToo long names could cause issues with certain games.");
                        }

                        this@MainController.OpenedAmiibo = amiibo;
                        this@MainController.OpenedAmiiboPath = path;

                        this@MainController.OpenedAmiiboNameText.setDisable(false);
                        this@MainController.OpenedAmiiboNameText.setText(amiibo.name);
                        this@MainController.OpenedAmiiboUseRandomUuidCheck.setDisable(false);
                        this@MainController.OpenedAmiiboUseRandomUuidCheck.setSelected(amiibo.use_random_uuid);
                        this@MainController.OpenedAmiiboAreaList.setDisable(false);

                        if(amiibo.hasAreas()) {
                            // todo
                            val area_items = mutableListOf<String>();
                            for(area in amiibo.areas!!.areas) {
                                var msg = "Game " + "0x%016X".format(area.program_id.toLong()) + " (" + "0x%08X".format(area.access_id.toInt()) + ")";
                                if(area.access_id == amiibo.areas!!.current_area_access_id) {
                                    msg = "[Active area] " + msg;
                                }
                                area_items.add(msg);
                            }
                            this@MainController.OpenedAmiiboAreaList.items.setAll(area_items);
                        }
                    }
                }
            }
        });

        this.AmiiboSaveButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                // Validate amiibo name
                val amiibo_name = this@MainController.OpenedAmiiboNameText.getText() as String;
                if(amiibo_name.isNullOrEmpty()) {
                    this@MainController.showError("The amiibo name is null or empty...");
                    return;
                }

                this@MainController.OpenedAmiibo!!.name = amiibo_name;
                this@MainController.OpenedAmiibo!!.use_random_uuid = this@MainController.OpenedAmiiboUseRandomUuidCheck.isSelected();
                if(this@MainController.OpenedAmiibo!!.save(this@MainController.OpenedAmiiboPath)) {
                    this@MainController.showInfo("The virtual amiibo was successfully updated!");

                    this@MainController.OpenedAmiibo = null;
                    this@MainController.OpenedAmiiboNameText.setText("");
                    this@MainController.OpenedAmiiboNameText.setDisable(true);
                    this@MainController.OpenedAmiiboUseRandomUuidCheck.setSelected(false);
                    this@MainController.OpenedAmiiboUseRandomUuidCheck.setDisable(true);
                    this@MainController.OpenedAmiiboAreaList.items.setAll(listOf<String>());
                    this@MainController.OpenedAmiiboAreaList.setDisable(true);
                }
                else {
                    this@MainController.showError("The virtual amiibo could not be updated...");
                }
            }
        });

        this.OpenedAmiiboNameText.textProperty().addListener(object : ChangeListener<String?> {
            override fun changed(a: ObservableValue<out String?>, old: String?, new: String?) {
                if(new!!.length > Amiibo.NameMaxLength) {
                    val str = new.substring(0, Amiibo.NameMaxLength);
                    this@MainController.OpenedAmiiboNameText.setText(str);
                }
            }
        });

        this.AboutButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                showInfo(this@MainController.MainStage.title);
                
                MainApplication.HostServices.showDocument("https://github.com/XorTroll/emuiibo");
            }
        });

        this.ApiReloadButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                this@MainController.tryReadApi(this@MainController.ApiUrlText.getText());
            }
        });
        
        this.ApiUrlText.setText(AmiiboAPI.DefaultUrl);

        this.GenerateOneAmiiboSeriesBox.selectionModel.selectedItemProperty().addListener(object : ChangeListener<String?> {
            override fun changed(a: ObservableValue<out String?>, old: String?, new: String?) {
                this@MainController.updateSelectedAmiiboSeries();
            }
        });

        this.AmiiboBox.selectionModel.selectedItemProperty().addListener(object : ChangeListener<String?> {
            override fun changed(a: ObservableValue<out String?>, old: String?, new: String?) {
                this@MainController.updateSelectedAmiibo();
            }
        });

        this.AmiiboNameText.textProperty().addListener(object : ChangeListener<String?> {
            override fun changed(a: ObservableValue<out String?>, old: String?, new: String?) {
                if(new!!.length > Amiibo.NameMaxLength) {
                    val str = new.substring(0, Amiibo.NameMaxLength);
                    this@MainController.AmiiboNameText.setText(str);
                }
            }
        });

        this.AmiiboDirectoryText.disableProperty().bind(this.NameAsDirectoryNameCheck.selectedProperty());

        this.GenerateOneFtpAddress.disableProperty().bind(this.GenerateOneFtpCheck.selectedProperty().not());
        this.GenerateOneFtpPort.disableProperty().bind(this.GenerateOneFtpCheck.selectedProperty().not());

        this.GenerateOneButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                this@MainController.cleanTemporaryFtp();

                val amiibo_name = this@MainController.AmiiboNameText.getText() as String;
                if(amiibo_name.isNullOrEmpty()) {
                    this@MainController.showError("The virtual amiibo name is null or empty...");
                    return;
                }

                val amiibo_dir = if(this@MainController.NameAsDirectoryNameCheck.isSelected()) {
                    amiibo_name
                }
                else {
                    this@MainController.AmiiboDirectoryText.getText()
                };
                if(amiibo_dir.isNullOrEmpty()) {
                    this@MainController.showError("The virtual amiibo directory is null or empty...");
                    return;
                }
                else if(amiibo_dir.contains("/") || amiibo_dir.contains("\\")) {
                    this@MainController.showError("The virtual amiibo directory contains invalid characters...");
                    return;
                }

                val is_ftp = this@MainController.GenerateOneFtpCheck.isSelected();
                val ftp_addr = this@MainController.GenerateOneFtpAddress.getText();
                val ftp_port = this@MainController.processFtpPort(is_ftp, this@MainController.GenerateOneFtpPort.getText());
                if(ftp_port == null) {
                    return;
                }

                val use_random_uuid = this@MainController.GenerateOneUseRandomUuidCheck.isSelected();
                val save_image = this@MainController.GenerateOneImageSaveCheck.isSelected();

                val paths = this@MainController.chooseBaseAmiiboPath(is_ftp);
                if(paths != null) {
                    val (path, pres_path) = paths;
                    val base_path = Utils.ensureValidFileDirectoryName(amiibo_dir);

                    val amiibo_path = Paths.get(path, base_path).toString();
                    val pres_amiibo_path = Paths.get(pres_path, base_path).toString();
                    if(this@MainController.showYesNo("The virtual amiibo will be generated at:\n" + pres_amiibo_path + "/{amiibo.json, amiibo.flag, ...}\n\nProceed with generation?")) {
                        val selected_amiibo = this@MainController.getSelectedAmiibo();
                        if(selected_amiibo != null) {
                            if(this@MainController.generateAmiibo(amiibo_path, base_path, selected_amiibo, amiibo_name, use_random_uuid, save_image, is_ftp, ftp_addr, ftp_port)) {
                                this@MainController.showInfo("The virtual amiibo was successfully generated!");
                            }
                            else {
                                this@MainController.showError("The virtual amiibo could not be generated...");
                            }
                        }
                        else {
                            this@MainController.showError("There is no virtual amiibo selected...");
                        }
                    }
                }
            }
        });

        this.GenerateAllFtpAddress.disableProperty().bind(this.GenerateAllFtpCheck.selectedProperty().not());
        this.GenerateAllFtpPort.disableProperty().bind(this.GenerateAllFtpCheck.selectedProperty().not());

        this.GenerateAllButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                this@MainController.cleanTemporaryFtp();

                val is_ftp = this@MainController.GenerateAllFtpCheck.isSelected();
                val ftp_addr = this@MainController.GenerateAllFtpAddress.getText();
                val ftp_port = this@MainController.processFtpPort(is_ftp, this@MainController.GenerateAllFtpPort.getText());
                if(ftp_port == null) {
                    return;
                }

                val use_random_uuid = this@MainController.GenerateAllUseRandomUuidCheck.isSelected();
                val save_image = this@MainController.GenerateAllImageSaveCheck.isSelected();

                val paths = this@MainController.chooseBaseAmiiboPath(is_ftp);
                if(paths != null) {
                    val (path, pres_path) = paths;
                    if(this@MainController.showYesNo("The virtual amiibos will be generated at:\n" + pres_path + "/<series>/<amiibo>/{amiibo.json, amiibo.flag, ...}\n\nProceed with generation?")) {
                        for(amiibos in this@MainController.Amiibos.values) {
                            for(amiibo in amiibos) {
                                val amiibo_name = Utils.produceAmiiboName(amiibo.amiibo_name);
                                val base_path = Paths.get(Utils.ensureValidFileDirectoryName(amiibo.series_name), Utils.ensureValidFileDirectoryName(amiibo.amiibo_name)).toString();

                                val amiibo_path = Paths.get(path, base_path).toString();
                                val pres_amiibo_path = Paths.get(pres_path, base_path).toString();
                                
                                if(this@MainController.generateAmiibo(amiibo_path, base_path, amiibo, amiibo_name, use_random_uuid, save_image, is_ftp, ftp_addr, ftp_port)) {
                                    System.out.println("Generated virtual amiibo ('" + amiibo_name + "'): '" + pres_amiibo_path + "'");
                                }
                                else {
                                    this@MainController.showError("The following virtual amiibo could not be generated:\n'" + pres_amiibo_path + "'");
                                }
                            }
                        }

                        this@MainController.showInfo("The virtual amiibos were successfully generated!");
                    }
                }
            }
        });

        this.GenerateSeriesFtpAddress.disableProperty().bind(this.GenerateSeriesFtpCheck.selectedProperty().not());
        this.GenerateSeriesFtpPort.disableProperty().bind(this.GenerateSeriesFtpCheck.selectedProperty().not());

        this.GenerateSeriesButton.setOnAction(object : EventHandler<ActionEvent> {
            override fun handle(event: ActionEvent) {
                this@MainController.cleanTemporaryFtp();

                val is_ftp = this@MainController.GenerateSeriesFtpCheck.isSelected();
                val ftp_addr = this@MainController.GenerateSeriesFtpAddress.getText();
                val ftp_port = this@MainController.processFtpPort(is_ftp, this@MainController.GenerateSeriesFtpPort.getText());
                if(ftp_port == null) {
                    return;
                }

                val use_random_uuid = this@MainController.GenerateSeriesUseRandomUuidCheck.isSelected();
                val save_image = this@MainController.GenerateSeriesImageSaveCheck.isSelected();

                val paths = this@MainController.chooseBaseAmiiboPath(is_ftp);
                if(paths != null) {
                    val (path, pres_path) = paths;
                    if(this@MainController.showYesNo("The virtual amiibos will be generated at:\n" + pres_path + "/<amiibo>/{amiibo.json, amiibo.flag, ...}\n\nProceed with generation?")) {
                        val series_name = this@MainController.GenerateSeriesAmiiboSeriesBox.selectionModel.selectedItem;
                        for(amiibo in this@MainController.Amiibos.get(series_name)!!) {
                            val amiibo_name = Utils.produceAmiiboName(amiibo.amiibo_name);
                            val base_path = Utils.ensureValidFileDirectoryName(amiibo.amiibo_name);

                            val amiibo_path = Paths.get(path, base_path).toString();
                            val pres_amiibo_path = Paths.get(pres_path, base_path).toString();
                            
                            if(this@MainController.generateAmiibo(amiibo_path, base_path, amiibo, amiibo_name, use_random_uuid, save_image, is_ftp, ftp_addr, ftp_port)) {
                                System.out.println("Generated virtual amiibo ('" + amiibo_name + "'): '" + pres_amiibo_path + "'");
                            }
                            else {
                                this@MainController.showError("The following virtual amiibo could not be generated:\n'" + pres_amiibo_path + "'");
                            }
                        }

                        this@MainController.showInfo("The virtual amiibos were successfully generated!");
                    }
                }
            }
        });
    }
}