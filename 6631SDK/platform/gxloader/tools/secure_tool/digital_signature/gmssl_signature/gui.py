#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
from signature import GmTool
from PyQt5.QtCore import pyqtSlot, Qt, QProcess
from PyQt5.QtWidgets import QProgressBar, QApplication, QDialog, QFileDialog, QMessageBox, QMenu, QAction, QInputDialog, QLineEdit
from PyQt5.QtGui import QCursor,  QPalette
from PyQt5.uic import loadUi
from configure import config
from common import *
import time
import subprocess
import shutil
import struct
from crc32 import crc32
from ota import ota
from version import version

class GmToolQt(GmTool, ota):
    def RunSubProcess(self, args, workdir=None):
        process = QProcess()
        if sys.platform != "win32":
            self.PrepareLinuxEnv()
        if workdir != None:
            process.setWorkingDirectory(workdir)
        cmdline = " ".join(args)
        process.start(cmdline)
        #logger.info(cmdline)
        #print(cmdline)
        ret = process.waitForFinished(10000)
        if ret != True:
            raise UsrError("run process timeout: %s"%args)
        retCode = process.exitCode()
        if retCode != 0:
            output = process.readAllStandardError()
            raise UsrError("run process error code: %d, %s: %s"%(retCode, cmdline, "%s"%bytes.decode(output.data())))
        output = process.readAllStandardOutput()
        msg = "%s"%bytes.decode(output.data())
        #print(msg)
        #logger.debug(msg)
        return msg

class GUIMain(QDialog):

    def __init__(self, *args):
        super(GUIMain, self).__init__(*args)
        self.ui = loadUi(resource_path('ui/signature.ui'), self)
        self.ui.setWindowFlags(Qt.Window)
        self.conf = config()
        self.config= self.conf.json
        self.lastOpenDir = os.path.dirname(self.config["sign_file"])
        try:
            content = ReadPartBytesOfFile(resource_path("bin/help.txt"))
            content = content.decode(encoding='UTF-8')
        except:
            self.ui.textBrowser.setText("打开帮助文件失败.")
        else:
            self.ui.textBrowser.setText(content)

        self.ui.setWindowTitle("signature " + version)
        self.ui.SignFileEdit.textChanged.connect(self.SignFileEdit_CheckText)
        self.ui.lineEditKernel.textChanged.connect(self.lineEditKernel_changed)
        self.ui.FlashAllEdit.textChanged.connect(self.FlashAllEdit_changed)

        self.ui.checkBox_AutoCheck.setChecked(self.config['auto_type'])
        self.ui.radioButton_NOR.setDisabled(self.config['auto_type'])
        self.ui.radioButton_NAND.setDisabled(self.config['auto_type'])
        self.ui.radioButton_CANOPUS.setDisabled(self.config['auto_type'])
        self.ui.radioButton_SIRIUS.setDisabled(self.config['auto_type'])

        self.ui.radioButton_NOR.setChecked(self.config['flash_type'] == 'nor')
        self.ui.radioButton_NAND.setChecked(self.config['flash_type'] == 'nand')
        self.ui.radioButton_CANOPUS.setChecked(self.config['chip_type'] == 'canopus')
        self.ui.radioButton_SIRIUS.setChecked(self.config['chip_type'] == 'sirius')

        self.ui.checkBox_AutoCheck.toggled.connect(self.checkboxChanged)
        self.ui.radioButton_NOR.toggled.connect(self.radioChanged)
        self.ui.radioButton_NAND.toggled.connect(self.radioChanged)
        self.ui.radioButton_CANOPUS.toggled.connect(self.radioChangedChipType)
        self.ui.radioButton_SIRIUS.toggled.connect(self.radioChangedChipType)

        self.ui.SignFileEdit.setText(self.config["sign_file"])
        self.ui.lineEditKernel.setText(self.config["kernel_file"])

        self.ui.FlashAllEdit.setText(self.config["flash_all_config"])

        self.ui.FillCheckBox.toggled.connect(self.FillCheckBoxChanged)
        self.ui.UpdateMRSCheckBox.toggled.connect(self.UpdateMRSCheckBoxChanged)
        if self.config['fill'] == False:
            self.ui.MaxLenlineEdit.setDisabled(True)
        self.ui.FillCheckBox.setChecked(self.config['fill'])
        self.ui.UpdateMRSCheckBox.setChecked(self.config['update_mrs'])
        self.ui.MaxLenlineEdit.setText(self.config["max_len"])
        self.ui.lineEditConf.setText(self.config["conf"])
        self.ui.MaxLenlineEdit.textChanged.connect(self.MaxLen_changed)

    def radioChanged(self, checked):
        if self.ui.radioButton_NOR.isChecked():
            self.config['flash_type'] = 'nor'

        if self.ui.radioButton_NAND.isChecked():
            self.config['flash_type'] = 'nand'

    def checkboxChanged(self, checked):
        if checked:
            self.config['auto_type'] = True
            self.ui.radioButton_NOR.setDisabled(True)
            self.ui.radioButton_NAND.setDisabled(True)
            self.ui.radioButton_CANOPUS.setDisabled(True)
            self.ui.radioButton_SIRIUS.setDisabled(True)
            self.SignFileEdit_CheckText(self.config['sign_file'])
        else:
            self.config['auto_type'] = False
            self.ui.radioButton_NOR.setDisabled(False)
            self.ui.radioButton_NAND.setDisabled(False)
            self.ui.radioButton_CANOPUS.setDisabled(False)
            self.ui.radioButton_SIRIUS.setDisabled(False)

    def radioChangedChipType(self, checked):
        if self.ui.radioButton_CANOPUS.isChecked():
            self.config['chip_type'] = 'canopus'

        if self.ui.radioButton_SIRIUS.isChecked():
            self.config['chip_type'] = 'sirius'

    def FillCheckBoxChanged(self, checked):
        if checked:
            self.config['fill'] = True
            self.ui.MaxLenlineEdit.setDisabled(False)
        else:
            self.config['fill'] = False
            self.ui.MaxLenlineEdit.setDisabled(True)

    def UpdateMRSCheckBoxChanged(self, checked):
        self.config['update_mrs'] = checked

    def closeEvent(self, event):
        self.conf.SaveJson(self.config)
        if os.path.exists("tmp"):
            shutil.rmtree("tmp")

    @pyqtSlot()
    def on_Sign_clicked(self):
        #dir_name = "signed_" + time.strftime("%Y%m%d_%H%M%S", time.localtime())
        self.ui.Sign.setEnabled(False)
        QApplication.processEvents()
        try:
            signFile = self.ui.SignFileEdit.text()
            dir_name = os.path.dirname(signFile)
            if signFile.strip() != "":
                self.config["sign_file"] = signFile
                gm = GmToolQt()
                data = gm.sign_bootloader_file(self.config['chip_type'], signFile, self.config['flash_type'])
        except UsrError as e:
            QMessageBox.critical(self, "错误",e.msg)
        except Exception as e:
            QMessageBox.critical(self, "错误", repr(e))
        else:
            try:
                SaveData(signFile, data)
            except:
                QMessageBox.critical(self, "错误", "保存%s文件失败"%signFile)
                return
            else:
                ret = QMessageBox.information(self, "签名完成", "签名文件在文件夹\n%s"%dir_name)
                """
                if ret == QMessageBox.Yes:
                    try:
                        if sys.platform == "win32":
                            os.system("start " + dir_name)
                        else:
                            subprocess.Popen(['nautilus', dir_name])
                    except:c
                        QMessageBox.critical(self, "错误", "打开%s文件夹失败"%dir_name)
                """
        self.ui.Sign.setEnabled(True)

    @pyqtSlot()
    def on_FLashAllBrowse_clicked(self):
        filename, filetype = QFileDialog.getOpenFileName(self, "请选择文件", self.lastOpenDir, "conf Files (*.conf);;All Files (*)")
        if filename != '':
            self.lastOpenDir = os.path.dirname(filename)
            self.ui.FlashAllEdit.setText(filename)
            self.config["flash_all_config"] = filename

    @pyqtSlot()
    def on_MRSGenPushButton_clicked(self):
        gm = GmToolQt()
        conf = self.config["flash_all_config"]
        workdir = os.path.dirname(conf)

        binTable = ParseConf(conf)

        binPath = {}
        binPath["HEAD"] = workdir + os.sep + binTable["HEAD"]
        binPath["TEE"] = workdir + os.sep + binTable["TEE"]
        binPath["KERNEL"] = workdir + os.sep + binTable["KERNEL"]
        binPath["HASH"] = workdir + os.sep + binTable["HASH"]

        try:
            mrsBytes = self.MakeMRS(binPath)
        except UsrError as e:
            QMessageBox.critical(self, "错误", e.msg)
            return
        except Exception:
            QMessageBox.critical(self, "错误", "生成Flash Head文件失败")
            return

        try:
            SaveData(binPath["HEAD"], mrsBytes)
            dir_name = os.path.dirname(binPath["HEAD"])
        except:
            QMessageBox.critical(self, "错误", "保存%s文件失败"%binPath["HEAD"])

        ret = QMessageBox.information(self, "完成", "文件在文件夹\n%s"%dir_name)
        """
        if ret == QMessageBox.Yes:
            try:
                if sys.platform == "win32":
                    os.system("start " + dir_name)
                else:
                    subprocess.Popen(['nautilus', dir_name])
            except:
                QMessageBox.critical(self, "错误", "打开%s文件夹失败"%dir_name)
        """

    def MakeMRS(self, binPath):
        MRSLEN = 768
        try:
            readbin = ReadPartBytesOfFile(binPath["HEAD"])
            if len(readbin) < MRSLEN:
                raise UsrError("读取MRS HEAD 出错, 小于%d字节"%MRSLEN)
            mrsBytes = bytearray(readbin)
        except Exception:
            raise UsrError("转换MRS Version出错")

        gm = GmToolQt()
        try:
            gm.sign_for_other_file(binPath["TEE"], gm.tmpDir + "trustedcore.sign")
        except Exception:
            raise UsrError("trustedcore签名出错")

        try:
            gm.sign_for_other_file(binPath["KERNEL"], gm.tmpDir + "kernel2.sign")
        except Exception:
            raise UsrError("kernel签名出错")

        try:
            gm.sign_for_other_file(binPath["HASH"], gm.tmpDir + "shatable.sign")
        except Exception:
            raise UsrError("shatable签名出错")


        mrsBytes[361:425] = ReadPartBytesOfFile(gm.tmpDir + "trustedcore.sign")
        mrsBytes[425:489] = ReadPartBytesOfFile(gm.tmpDir + "kernel2.sign")
        mrsBytes[489:553] = ReadPartBytesOfFile(gm.tmpDir + "shatable.sign")

        crc = crc32(mrsBytes[: (MRSLEN-4)])
        mrsBytes[(MRSLEN-4):MRSLEN]= struct.pack('I', crc)

        DeleteFile(gm.tmpDir + "trustedcore.sign")
        DeleteFile(gm.tmpDir + "kernel2.sign")
        DeleteFile(gm.tmpDir + "shatable.sign")

        return mrsBytes[:MRSLEN]

    @pyqtSlot()
    def on_signKernel_clicked(self):
        dir_name = "signed_" + time.strftime("%Y%m%d_%H%M%S", time.localtime())
        try:
            signFile = self.ui.lineEditKernel.text()
            if signFile.strip() != "":
                name = os.path.basename(signFile)
                output_path = dir_name + os.sep + name
                if not name.endswith(".sign"):
                    output_path += ".sign"
                gm = GmToolQt()
                gm.sign_for_other_file(signFile, output_path)
        except UsrError as e:
            QMessageBox.critical(self, "错误",e.msg)
        except Exception as e:
            QMessageBox.critical(self, "错误", repr(e))
        else:
            ret = QMessageBox.question(self, "签名完成", "签名文件在文件夹\n%s\n 是否打开签名文件夹?"%dir_name)
            if ret == QMessageBox.Yes:
                try:
                    if sys.platform == "win32":
                        os.system("start explorer .\\" + dir_name)
                    else:
                        subprocess.Popen(['nautilus', dir_name])
                except:
                    QMessageBox.critical(self, "错误", "打开%s文件夹失败"%dir_name)

    @pyqtSlot()
    def on_SignFileBrowse_clicked(self):
        filename, filetype = QFileDialog.getOpenFileName(self, "请选择文件", self.lastOpenDir, "Bin Files (*.bin);;All Files (*)")
        if filename != '':
            self.lastOpenDir = os.path.dirname(filename)
            self.ui.SignFileEdit.setText(filename)

    @pyqtSlot()
    def on_kernelBrowse_clicked(self):
        filename, filetype = QFileDialog.getOpenFileName(self, "请选择文件", self.lastOpenDir, "Bin Files (*.bin);;All Files (*)")
        if filename != '':
            self.lastOpenDir = os.path.dirname(filename)
            self.ui.lineEditKernel.setText(filename)

    def lineEditKernel_changed(self, text):
        self.config['kernel_file'] = text

    def FlashAllEdit_changed(self, text):
        self.config['flash_all_config'] = text
        self.SetMRSVerEdit()

    def MaxLen_changed(self, text):
        self.config['max_len'] = text

    def SetMRSVerEdit(self):
        try:
            gm = GmToolQt()

            workdir = os.path.dirname(self.config['flash_all_config'])
            binTable = ParseConf(self.config['flash_all_config'])

            path = workdir + os.sep + binTable["HEAD"]
            ver = ReadPartBytesOfFile(path, 0, 2)

        except:
            self.ui.MRSVerEdit.setText("NULL")
            return

        if len(ver) != 2:
            self.ui.MRSVerEdit.setText("NULL")
            return

        self.ui.MRSVerEdit.setText("%02x%02x"%(ver[1], ver[0]))

    def SignFileEdit_CheckText(self, text):
        self.config['sign_file'] = text

        if self.config['auto_type'] == False:
            return

        try:
            magic = ReadPartBytesOfFile(text, 0, 4096)
            if len(magic) < 4096:
                QMessageBox.critical(self, "错误","bootloader文件大小不对!")
                return
            else:
                if all(data == 0xFF for data in magic[1024+56:4095]):
                    pagesize = 4096
                    self.ui.radioButton_CANOPUS.setChecked(True)
                    self.ui.radioButton_NAND.setChecked(True)
                    return
                elif all(data == 0xFF for data in magic[1024+56:2047]):
                    pagesize = 2048
                    self.ui.radioButton_CANOPUS.setChecked(True)
                    self.ui.radioButton_NAND.setChecked(True)
                    return

            self.ui.radioButton_SIRIUS.setChecked(True)
            magic = ReadPartBytesOfFile(text, 0, 4)

            if magic == b'\xbb\x55\xbb\x55':
                print("spi nand firmware")
                self.ui.radioButton_NAND.setChecked(True)
            elif magic == b'\xaa\x55\xaa\x55':
                if is5CopyFirmware(text):
                    print("nand firmware")
                    self.ui.radioButton_NAND.setChecked(True)
                else:
                    print("spi nor firmware")
                    self.ui.radioButton_NOR.setChecked(True)
            elif magic == b'\x74\x6f\x6f\x62':
                print("boot firmware")
                self.ui.radioButton_NOR.setChecked(True)
            else:
                print("unknown firmware")
        except:
            pass

    @pyqtSlot()
    def on_GenKey_clicked(self):
        try:
            gm = GmToolQt()
            #dir_name = "gm_key_" + time.strftime("%Y%m%d_%H%M%S", time.localtime())

            dir_name = "key"

            reply = QMessageBox.question(self, '提示', '将会覆盖key文件下的密钥文件，是否继续？',
                                               QMessageBox.Yes, QMessageBox.No)
            if reply != QMessageBox.Yes:
                return

            gm.generate_private_key(gm.tmpDir + "blk0_pri.pem")
            gm.generate_private_bin(gm.tmpDir + "blk0_pri.pem", dir_name + os.sep + "blk0_pri.bin")
            gm.generate_public_bin(gm.tmpDir + "blk0_pri.pem", dir_name + os.sep + "blk0_pub.bin")
            DeleteFile(gm.tmpDir + "blk0_pri.pem")

            gm.generate_private_key(gm.tmpDir + "blk1_pri.pem")
            gm.generate_private_bin(gm.tmpDir + "blk1_pri.pem", dir_name + os.sep + "blk1_pri.bin")
            gm.generate_public_bin(gm.tmpDir + "blk1_pri.pem", dir_name + os.sep + "blk1_pub.bin")
            DeleteFile(gm.tmpDir + "blk1_pri.pem")
        except UsrError as e:
            QMessageBox.critical(self, "错误",e.msg)
        except Exception as e:
            QMessageBox.critical(self, "错误", repr(e))
        else:
            ret = QMessageBox.question(self, "生成密钥完成", "密钥文件在文件夹 %s\n 是否打开密钥文件夹?"%dir_name)
            if ret == QMessageBox.Yes:
                try:
                    if sys.platform == "win32":
                        os.system("start explorer .\\" + dir_name)
                    else:
                        subprocess.Popen(['nautilus ', dir_name])
                except:
                    QMessageBox.critical(self, "错误", "打开%s文件夹失败"%dir_name)

    @pyqtSlot()
    def on_ConfBrowse_clicked(self):
        filename, filetype = QFileDialog.getOpenFileName(self, "请选择文件", self.lastOpenDir, "conf Files (*.conf);;All Files (*)")
        if filename != '':
            self.lastOpenDir = os.path.dirname(filename)
            self.ui.lineEditConf.setText(filename)
            self.config["conf"] = filename

    @pyqtSlot()
    def on_MergeSign_clicked(self):
        if self.ui.FillCheckBox.isChecked():
            try:
                max_len = int(self.ui.MaxLenlineEdit.text())
            except:
                QMessageBox.critical(self, "错误", "转换最大长度失败")
                return

        gm = GmToolQt()
        conf = self.ui.lineEditConf.text()
        self.config["conf"] = conf
        workdir = os.path.dirname(conf)

        binTable = ParseConf(conf)

        binPath = {}
        binPath["HEAD"] = workdir + os.sep + binTable["HEAD"]
        binPath["TEE"] = workdir + os.sep + binTable["TEE"]
        binPath["KERNEL"] = workdir + os.sep + binTable["KERNEL"]
        binPath["HASH"] = workdir + os.sep + binTable["HASH"]

        if self.ui.UpdateMRSCheckBox.isChecked():
            try:
                mrsBytes = self.MakeMRS(binPath)
            except UsrError as e:
                QMessageBox.critical(self, "错误", e.msg)
                return
            except Exception:
                QMessageBox.critical(self, "错误", "生成Flash Head文件失败")
                return

            try:
                SaveData(binPath["HEAD"], mrsBytes)
            except:
                QMessageBox.critical(self, "错误", "保存Flash Head文件失败\n%s"%binPath["HEAD"])
                return

        try:
            ver = ReadPartBytesOfFile(binPath["HEAD"], 0 , 2)
        except UsrError as e:
            QMessageBox.critical(self, "错误", e.msg)
            return
        except:
            QMessageBox.critical(self, "错误", "打开Flash Head文件失败\n%s"%binPath["HEAD"])
            return

        mrsVer = "%02x%02x"%(ver[1], ver[0])

        dirname = workdir + os.sep + "GX3215B_OTA_" + mrsVer + \
                time.strftime("_%Y%m%d%H%M%S", time.localtime())
        os.makedirs(dirname)
        output = dirname + os.sep + "GX3215B_OTA_" + mrsVer + ".bin.merge"
        gm.genflash_merge(conf, output)
        CheckGenFileOK(output)

        if self.ui.FillCheckBox.isChecked():
            max_buffer_len = 2 * 1024 * 1024
            max_len = max_len * 1024 * 1024
            print("max len 0x%x"%max_len)
            mergeLen = os.path.getsize(output)
            print("data len 0x%x"%(mergeLen))
            fillLen = max_len - mergeLen
            print("fill len 0x%x"%fillLen)

            f = open(output, 'ab')

            reserved = bytearray(max_buffer_len)

            for i in range(max_buffer_len):
                reserved[i] = 0x00

            while (fillLen > 0):
                if fillLen >= max_buffer_len:
                    f.write(reserved)
                else:
                    f.write(reserved[0:fillLen])
                fillLen = fillLen - max_buffer_len
            f.flush()
            f.close()

        signFile = dirname + os.sep + "GX3215B_OTA_" + mrsVer + ".sign"
        gm.sign_for_other_file(output, signFile)

        ret = QMessageBox.information(self, "OTA生成完成", "文件在文件夹\n%s"%dirname)
        """"
        if ret == QMessageBox.Yes:
            try:
                if sys.platform == "win32":
                    os.system("start " + dirname)
                else:
                    subprocess.Popen(['nautilus ', dirname])
            except:
                QMessageBox.critical(self, "错误", "打开%s文件夹失败"%dirname)
        """

if __name__ == '__main__':
    app = QApplication(sys.argv)
    widget = GUIMain()
    widget.show()
    sys.exit(app.exec_())

