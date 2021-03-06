﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// ------------------------------------------------------------------------------------------

//以下warning C4100を黙らせる
//C4100 : 引数は関数の本体部で 1 度も参照されません。
#pragma warning( push )
#pragma warning( disable: 4100 )

#include "auo_version.h"
#include "auo_frm.h"
#include "auo_faw2aac.h"
#include "frmConfig.h"
#include "frmSaveNewStg.h"
#include "frmOtherSettings.h"
#include "frmBitrateCalculator.h"

#include "cpu_info.h"
#include "gpu_info.h"
#include "NVEncUtil.h"

using namespace NVEnc;

/// -------------------------------------------------
///     設定画面の表示
/// -------------------------------------------------
[STAThreadAttribute]
void ShowfrmConfig(CONF_GUIEX *conf, const SYSTEM_DATA *sys_dat) {
    if (!sys_dat->exstg->s_local.disable_visual_styles)
        System::Windows::Forms::Application::EnableVisualStyles();
    System::IO::Directory::SetCurrentDirectory(String(sys_dat->aviutl_dir).ToString());
    frmConfig frmConf(conf, sys_dat);
    frmConf.ShowDialog();
}

/// -------------------------------------------------
///     frmSaveNewStg 関数
/// -------------------------------------------------
System::Boolean frmSaveNewStg::checkStgFileName(String^ stgName) {
    String^ fileName;
    if (stgName->Length == 0)
        return false;
    
    if (!ValidiateFileName(stgName)) {
        MessageBox::Show(L"ファイル名に使用できない文字が含まれています。\n保存できません。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
        return false;
    }
    if (String::Compare(Path::GetExtension(stgName), L".stg", true))
        stgName += L".stg";
    if (File::Exists(fileName = Path::Combine(fsnCXFolderBrowser->GetSelectedFolder(), stgName)))
        if (MessageBox::Show(stgName + L" はすでに存在します。上書きしますか?", L"上書き確認", MessageBoxButtons::YesNo, MessageBoxIcon::Question)
            != System::Windows::Forms::DialogResult::Yes)
            return false;
    StgFileName = fileName;
    return true;
}

System::Void frmSaveNewStg::setStgDir(String^ _stgDir) {
    StgDir = _stgDir;
    fsnCXFolderBrowser->SetRootDirAndReload(StgDir);
}


/// -------------------------------------------------
///     frmBitrateCalculator 関数
/// -------------------------------------------------
System::Void frmBitrateCalculator::Init(int VideoBitrate, int AudioBitrate, bool BTVBEnable, bool BTABEnable, int ab_max) {
    guiEx_settings exStg(true);
    exStg.load_fbc();
    enable_events = false;
    fbcTXSize->Text = exStg.s_fbc.initial_size.ToString("F2");
    fbcChangeTimeSetMode(exStg.s_fbc.calc_time_from_frame != 0);
    fbcRBCalcRate->Checked = exStg.s_fbc.calc_bitrate != 0;
    fbcRBCalcSize->Checked = !fbcRBCalcRate->Checked;
    fbcTXMovieFrameRate->Text = Convert::ToString(exStg.s_fbc.last_fps);
    fbcNUMovieFrames->Value = exStg.s_fbc.last_frame_num;
    fbcNULengthHour->Value = Convert::ToDecimal((int)exStg.s_fbc.last_time_in_sec / 3600);
    fbcNULengthMin->Value = Convert::ToDecimal((int)(exStg.s_fbc.last_time_in_sec % 3600) / 60);
    fbcNULengthSec->Value =  Convert::ToDecimal((int)exStg.s_fbc.last_time_in_sec % 60);
    SetBTVBEnabled(BTVBEnable);
    SetBTABEnabled(BTABEnable, ab_max);
    SetNUVideoBitrate(VideoBitrate);
    SetNUAudioBitrate(AudioBitrate);
    enable_events = true;
}
System::Void frmBitrateCalculator::frmBitrateCalculator_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
    guiEx_settings exStg(true);
    exStg.load_fbc();
    exStg.s_fbc.calc_bitrate = fbcRBCalcRate->Checked;
    exStg.s_fbc.calc_time_from_frame = fbcPNMovieFrames->Visible;
    exStg.s_fbc.last_fps = Convert::ToDouble(fbcTXMovieFrameRate->Text);
    exStg.s_fbc.last_frame_num = Convert::ToInt32(fbcNUMovieFrames->Value);
    exStg.s_fbc.last_time_in_sec = Convert::ToInt32(fbcNULengthHour->Value) * 3600
                                 + Convert::ToInt32(fbcNULengthMin->Value) * 60
                                 + Convert::ToInt32(fbcNULengthSec->Value);
    if (fbcRBCalcRate->Checked)
        exStg.s_fbc.initial_size = Convert::ToDouble(fbcTXSize->Text);
    exStg.save_fbc();
    frmConfig^ fcg = dynamic_cast<frmConfig^>(this->Owner);
    if (fcg != nullptr)
        fcg->InformfbcClosed();
}
System::Void frmBitrateCalculator::fbcRBCalcRate_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
    if (fbcRBCalcRate->Checked && Convert::ToDouble(fbcTXSize->Text) <= 0.0) {
        guiEx_settings exStg(true);
        exStg.load_fbc();
        fbcTXSize->Text = exStg.s_fbc.initial_size.ToString("F2");
    }
}
System::Void frmBitrateCalculator::fbcBTVBApply_Click(System::Object^  sender, System::EventArgs^  e) {
    frmConfig^ fcg = dynamic_cast<frmConfig^>(this->Owner);
    if (fcg != nullptr)
        fcg->SetVideoBitrate((int)fbcNUBitrateVideo->Value);
}
System::Void frmBitrateCalculator::fbcBTABApply_Click(System::Object^  sender, System::EventArgs^  e) {
    frmConfig^ fcg = dynamic_cast<frmConfig^>(this->Owner);
    if (fcg != nullptr)
        fcg->SetAudioBitrate((int)fbcNUBitrateAudio->Value);
}


/// -------------------------------------------------
///     frmConfig 関数  (frmBitrateCalculator関連)
/// -------------------------------------------------
System::Void frmConfig::CloseBitrateCalc() {
    frmBitrateCalculator::Instance::get()->Close();
}
System::Void frmConfig::fcgTSBBitrateCalc_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
    if (fcgTSBBitrateCalc->Checked) {
        bool videoBitrateMode = true;

        frmBitrateCalculator::Instance::get()->Init(
            (int)fcgNUBitrate->Value,
            (fcgNUAudioBitrate->Visible) ? (int)fcgNUAudioBitrate->Value : 0,
            videoBitrateMode,
            fcgNUAudioBitrate->Visible,
            (int)fcgNUAudioBitrate->Maximum
            );
        frmBitrateCalculator::Instance::get()->Owner = this;
        frmBitrateCalculator::Instance::get()->Show();
    } else {
        frmBitrateCalculator::Instance::get()->Close();
    }
}
System::Void frmConfig::SetfbcBTABEnable(bool enable, int max) {
    frmBitrateCalculator::Instance::get()->SetBTABEnabled(fcgNUAudioBitrate->Visible, max);
}
System::Void frmConfig::SetfbcBTVBEnable(bool enable) {
    frmBitrateCalculator::Instance::get()->SetBTVBEnabled(enable);
}

System::Void frmConfig::SetVideoBitrate(int bitrate) {
    SetNUValue(fcgNUBitrate, bitrate);
}

System::Void frmConfig::SetAudioBitrate(int bitrate) {
    SetNUValue(fcgNUAudioBitrate, bitrate);
}
System::Void frmConfig::InformfbcClosed() {
    fcgTSBBitrateCalc->Checked = false;
}


/// -------------------------------------------------
///     frmConfig 関数
/// -------------------------------------------------
/////////////   LocalStg関連  //////////////////////
System::Void frmConfig::LoadLocalStg() {
    guiEx_settings *_ex_stg = sys_dat->exstg;
    _ex_stg->load_encode_stg();
    LocalStg.CustomTmpDir    = String(_ex_stg->s_local.custom_tmp_dir).ToString();
    LocalStg.CustomAudTmpDir = String(_ex_stg->s_local.custom_audio_tmp_dir).ToString();
    LocalStg.CustomMP4TmpDir = String(_ex_stg->s_local.custom_mp4box_tmp_dir).ToString();
    LocalStg.LastAppDir      = String(_ex_stg->s_local.app_dir).ToString();
    LocalStg.LastBatDir      = String(_ex_stg->s_local.bat_dir).ToString();
    LocalStg.MP4MuxerExeName = String(_ex_stg->s_mux[MUXER_MP4].filename).ToString();
    LocalStg.MP4MuxerPath    = String(_ex_stg->s_mux[MUXER_MP4].fullpath).ToString();
    LocalStg.MKVMuxerExeName = String(_ex_stg->s_mux[MUXER_MKV].filename).ToString();
    LocalStg.MKVMuxerPath    = String(_ex_stg->s_mux[MUXER_MKV].fullpath).ToString();
    LocalStg.TC2MP4ExeName   = String(_ex_stg->s_mux[MUXER_TC2MP4].filename).ToString();
    LocalStg.TC2MP4Path      = String(_ex_stg->s_mux[MUXER_TC2MP4].fullpath).ToString();
    LocalStg.MPGMuxerExeName = String(_ex_stg->s_mux[MUXER_MPG].filename).ToString();
    LocalStg.MPGMuxerPath    = String(_ex_stg->s_mux[MUXER_MPG].fullpath).ToString();
    LocalStg.MP4RawExeName   = String(_ex_stg->s_mux[MUXER_MP4_RAW].filename).ToString();
    LocalStg.MP4RawPath      = String(_ex_stg->s_mux[MUXER_MP4_RAW].fullpath).ToString();

    LocalStg.audEncName->Clear();
    LocalStg.audEncExeName->Clear();
    LocalStg.audEncPath->Clear();
    for (int i = 0; i < _ex_stg->s_aud_count; i++) {
        LocalStg.audEncName->Add(String(_ex_stg->s_aud[i].dispname).ToString());
        LocalStg.audEncExeName->Add(String(_ex_stg->s_aud[i].filename).ToString());
        LocalStg.audEncPath->Add(String(_ex_stg->s_aud[i].fullpath).ToString());
    }
    if (_ex_stg->s_local.large_cmdbox)
        fcgTXCmd_DoubleClick(nullptr, nullptr); //初期状態は縮小なので、拡大
}

System::Boolean frmConfig::CheckLocalStg() {
    bool error = false;
    String^ err = "";
    //音声エンコーダのチェック (実行ファイル名がない場合はチェックしない)
    if (LocalStg.audEncExeName[fcgCXAudioEncoder->SelectedIndex]->Length) {
        String^ AudioEncoderPath = LocalStg.audEncPath[fcgCXAudioEncoder->SelectedIndex];
        if (!File::Exists(AudioEncoderPath) 
            && (fcgCXAudioEncoder->SelectedIndex != sys_dat->exstg->s_aud_faw_index 
                || !check_if_faw2aac_exists()) ) {
            //音声実行ファイルがない かつ
            //選択された音声がfawでない または fawであってもfaw2aacがない
            if (!error) err += L"\n\n";
            error = true;
            err += L"指定された 音声エンコーダ は存在しません。\n [ " + AudioEncoderPath + L" ]\n";
        }
    }
    //FAWのチェック
    if (fcgCBFAWCheck->Checked) {
        if (sys_dat->exstg->s_aud_faw_index == FAW_INDEX_ERROR) {
            if (!error) err += L"\n\n";
            error = true;
            err += L"FAWCheckが選択されましたが、NVEnc.ini から\n"
                + L"FAW の設定を読み込めませんでした。\n"
                + L"NVEnc.ini を確認してください。\n";
        } else if (!File::Exists(LocalStg.audEncPath[sys_dat->exstg->s_aud_faw_index])
                   && !check_if_faw2aac_exists()) {
            //fawの実行ファイルが存在しない かつ faw2aacも存在しない
            if (!error) err += L"\n\n";
            error = true;
            err += L"FAWCheckが選択されましたが、FAW(fawcl)へのパスが正しく指定されていません。\n"
                +  L"一度設定画面でFAW(fawcl)へのパスを指定してください。\n";
        }
    }
    if (error) 
        MessageBox::Show(this, err, L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
    return error;
}

System::Void frmConfig::SaveLocalStg() {
    guiEx_settings *_ex_stg = sys_dat->exstg;
    _ex_stg->load_encode_stg();
    _ex_stg->s_local.large_cmdbox = fcgTXCmd->Multiline;
    GetCHARfromString(_ex_stg->s_local.custom_tmp_dir,        sizeof(_ex_stg->s_local.custom_tmp_dir),        LocalStg.CustomTmpDir);
    GetCHARfromString(_ex_stg->s_local.custom_mp4box_tmp_dir, sizeof(_ex_stg->s_local.custom_mp4box_tmp_dir), LocalStg.CustomMP4TmpDir);
    GetCHARfromString(_ex_stg->s_local.custom_audio_tmp_dir,  sizeof(_ex_stg->s_local.custom_audio_tmp_dir),  LocalStg.CustomAudTmpDir);
    GetCHARfromString(_ex_stg->s_local.app_dir,               sizeof(_ex_stg->s_local.app_dir),               LocalStg.LastAppDir);
    GetCHARfromString(_ex_stg->s_local.bat_dir,               sizeof(_ex_stg->s_local.bat_dir),               LocalStg.LastBatDir);
    GetCHARfromString(_ex_stg->s_mux[MUXER_MP4].fullpath,     sizeof(_ex_stg->s_mux[MUXER_MP4].fullpath),     LocalStg.MP4MuxerPath);
    GetCHARfromString(_ex_stg->s_mux[MUXER_MKV].fullpath,     sizeof(_ex_stg->s_mux[MUXER_MKV].fullpath),     LocalStg.MKVMuxerPath);
    GetCHARfromString(_ex_stg->s_mux[MUXER_TC2MP4].fullpath,  sizeof(_ex_stg->s_mux[MUXER_TC2MP4].fullpath),  LocalStg.TC2MP4Path);
    GetCHARfromString(_ex_stg->s_mux[MUXER_MPG].fullpath,     sizeof(_ex_stg->s_mux[MUXER_MPG].fullpath),     LocalStg.MPGMuxerPath);
    GetCHARfromString(_ex_stg->s_mux[MUXER_MP4_RAW].fullpath, sizeof(_ex_stg->s_mux[MUXER_MP4_RAW].fullpath), LocalStg.MP4RawPath);
    for (int i = 0; i < _ex_stg->s_aud_count; i++)
        GetCHARfromString(_ex_stg->s_aud[i].fullpath,         sizeof(_ex_stg->s_aud[i].fullpath),             LocalStg.audEncPath[i]);
    _ex_stg->save_local();
}

System::Void frmConfig::SetLocalStg() {
    fcgTXMP4MuxerPath->Text       = LocalStg.MP4MuxerPath;
    fcgTXMKVMuxerPath->Text       = LocalStg.MKVMuxerPath;
    fcgTXTC2MP4Path->Text         = LocalStg.TC2MP4Path;
    fcgTXMPGMuxerPath->Text       = LocalStg.MPGMuxerPath;
    fcgTXMP4RawPath->Text         = LocalStg.MP4RawPath;
    fcgTXCustomAudioTempDir->Text = LocalStg.CustomAudTmpDir;
    fcgTXCustomTempDir->Text      = LocalStg.CustomTmpDir;
    fcgTXMP4BoxTempDir->Text      = LocalStg.CustomMP4TmpDir;
    fcgLBMP4MuxerPath->Text       = LocalStg.MP4MuxerExeName + L" の指定";
    fcgLBMKVMuxerPath->Text       = LocalStg.MKVMuxerExeName + L" の指定";
    fcgLBTC2MP4Path->Text         = LocalStg.TC2MP4ExeName   + L" の指定";
    fcgLBMPGMuxerPath->Text       = LocalStg.MPGMuxerExeName + L" の指定";
    fcgLBMP4RawPath->Text         = LocalStg.MP4RawExeName + L" の指定";

    fcgTXMP4MuxerPath->SelectionStart     = fcgTXMP4MuxerPath->Text->Length;
    fcgTXTC2MP4Path->SelectionStart       = fcgTXTC2MP4Path->Text->Length;
    fcgTXMKVMuxerPath->SelectionStart     = fcgTXMKVMuxerPath->Text->Length;
    fcgTXMPGMuxerPath->SelectionStart     = fcgTXMPGMuxerPath->Text->Length;
    fcgTXMP4RawPath->SelectionStart       = fcgTXMP4RawPath->Text->Length;
}

//////////////       その他イベント処理   ////////////////////////
System::Void frmConfig::ActivateToolTip(bool Enable) {
    fcgTTEx->Active = Enable;
}

System::Void frmConfig::fcgTSBOtherSettings_Click(System::Object^  sender, System::EventArgs^  e) {
    frmOtherSettings::Instance::get()->stgDir = String(sys_dat->exstg->s_local.stg_dir).ToString();
    frmOtherSettings::Instance::get()->ShowDialog();
    char buf[MAX_PATH_LEN];
    GetCHARfromString(buf, sizeof(buf), frmOtherSettings::Instance::get()->stgDir);
    if (_stricmp(buf, sys_dat->exstg->s_local.stg_dir)) {
        //変更があったら保存する
        strcpy_s(sys_dat->exstg->s_local.stg_dir, sizeof(sys_dat->exstg->s_local.stg_dir), buf);
        sys_dat->exstg->save_local();
        InitStgFileList();
    }
    //再読み込み
    guiEx_settings stg;
    stg.load_encode_stg();
    log_reload_settings();
    sys_dat->exstg->s_local.get_relative_path = stg.s_local.get_relative_path;
    SetStgEscKey(stg.s_local.enable_stg_esc_key != 0);
    ActivateToolTip(stg.s_local.disable_tooltip_help == FALSE);
    if (str_has_char(stg.s_local.conf_font.name))
        SetFontFamilyToForm(this, gcnew FontFamily(String(stg.s_local.conf_font.name).ToString()), this->Font->FontFamily);
}
System::Boolean frmConfig::EnableSettingsNoteChange(bool Enable) {
    if (fcgTSTSettingsNotes->Visible == Enable &&
        fcgTSLSettingsNotes->Visible == !Enable)
        return true;
    if (CountStringBytes(fcgTSTSettingsNotes->Text) > fcgTSTSettingsNotes->MaxLength - 1) {
        MessageBox::Show(this, L"入力された文字数が多すぎます。減らしてください。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
        fcgTSTSettingsNotes->Focus();
        fcgTSTSettingsNotes->SelectionStart = fcgTSTSettingsNotes->Text->Length;
        return false;
    }
    fcgTSTSettingsNotes->Visible = Enable;
    fcgTSLSettingsNotes->Visible = !Enable;
    if (Enable) {
        fcgTSTSettingsNotes->Text = fcgTSLSettingsNotes->Text;
        fcgTSTSettingsNotes->Focus();
        bool isDefaultNote = String::Compare(fcgTSTSettingsNotes->Text, String(DefaultStgNotes).ToString()) == 0;
        fcgTSTSettingsNotes->Select((isDefaultNote) ? 0 : fcgTSTSettingsNotes->Text->Length, fcgTSTSettingsNotes->Text->Length);
    } else {
        SetfcgTSLSettingsNotes(fcgTSTSettingsNotes->Text);
        CheckOtherChanges(nullptr, nullptr);
    }
    return true;
}
System::Void frmConfig::fcgTSLSettingsNotes_DoubleClick(System::Object^  sender, System::EventArgs^  e) {
    EnableSettingsNoteChange(true);
}
System::Void frmConfig::fcgTSTSettingsNotes_Leave(System::Object^  sender, System::EventArgs^  e) {
    EnableSettingsNoteChange(false);
}
System::Void frmConfig::fcgTSTSettingsNotes_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
    if (e->KeyCode == Keys::Return)
        EnableSettingsNoteChange(false);
}
System::Void frmConfig::fcgTSTSettingsNotes_TextChanged(System::Object^  sender, System::EventArgs^  e) {
    SetfcgTSLSettingsNotes(fcgTSTSettingsNotes->Text);
    CheckOtherChanges(nullptr, nullptr);
}

/////////////    音声設定関連の関数    ///////////////
System::Void frmConfig::fcgCBAudio2pass_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
    if (fcgCBAudio2pass->Checked) {
        fcgCBAudioUsePipe->Checked = false;
        fcgCBAudioUsePipe->Enabled = false;
    } else if (CurrentPipeEnabled) {
        fcgCBAudioUsePipe->Checked = true;
        fcgCBAudioUsePipe->Enabled = true;
    }
}

System::Void frmConfig::fcgCXAudioEncoder_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
    setAudioDisplay();
}

System::Void frmConfig::fcgCXAudioEncMode_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
    AudioEncodeModeChanged();
}

System::Int32 frmConfig::GetCurrentAudioDefaultBitrate() {
    return sys_dat->exstg->s_aud[fcgCXAudioEncoder->SelectedIndex].mode[fcgCXAudioEncMode->SelectedIndex].bitrate_default;
}

System::Void frmConfig::setAudioDisplay() {
    AUDIO_SETTINGS *astg = &sys_dat->exstg->s_aud[fcgCXAudioEncoder->SelectedIndex];
    //～の指定
    if (str_has_char(astg->filename)) {
        fcgLBAudioEncoderPath->Text = String(astg->filename).ToString() + L" の指定";
        fcgTXAudioEncoderPath->Enabled = true;
        fcgTXAudioEncoderPath->Text = LocalStg.audEncPath[fcgCXAudioEncoder->SelectedIndex];
        fcgBTAudioEncoderPath->Enabled = true;
    } else {
        //filename空文字列(wav出力時)
        fcgLBAudioEncoderPath->Text = L"";
        fcgTXAudioEncoderPath->Enabled = false;
        fcgTXAudioEncoderPath->Text = L"";
        fcgBTAudioEncoderPath->Enabled = false;
    }
    fcgTXAudioEncoderPath->SelectionStart = fcgTXAudioEncoderPath->Text->Length;
    fcgCXAudioEncMode->BeginUpdate();
    fcgCXAudioEncMode->Items->Clear();
    for (int i = 0; i < astg->mode_count; i++)
        fcgCXAudioEncMode->Items->Add(String(astg->mode[i].name).ToString());
    fcgCXAudioEncMode->EndUpdate();
    bool pipe_enabled = (astg->pipe_input && (!(fcgCBAudio2pass->Checked && astg->mode[fcgCXAudioEncMode->SelectedIndex].enc_2pass != 0)));
    CurrentPipeEnabled = pipe_enabled;
    fcgCBAudioUsePipe->Enabled = pipe_enabled;
    fcgCBAudioUsePipe->Checked = pipe_enabled;
    if (fcgCXAudioEncMode->Items->Count > 0)
        fcgCXAudioEncMode->SelectedIndex = 0;
}

System::Void frmConfig::AudioEncodeModeChanged() {
    int index = fcgCXAudioEncMode->SelectedIndex;
    AUDIO_SETTINGS *astg = &sys_dat->exstg->s_aud[fcgCXAudioEncoder->SelectedIndex];
    if (astg->mode[index].bitrate) {
        fcgCXAudioEncMode->Width = fcgCXAudioEncModeSmallWidth;
        fcgLBAudioBitrate->Visible = true;
        fcgNUAudioBitrate->Visible = true;
        fcgNUAudioBitrate->Minimum = astg->mode[index].bitrate_min;
        fcgNUAudioBitrate->Maximum = astg->mode[index].bitrate_max;
        fcgNUAudioBitrate->Increment = astg->mode[index].bitrate_step;
        SetNUValue(fcgNUAudioBitrate, (conf->aud.bitrate != 0) ? conf->aud.bitrate : astg->mode[index].bitrate_default);
    } else {
        fcgCXAudioEncMode->Width = fcgCXAudioEncModeLargeWidth;
        fcgLBAudioBitrate->Visible = false;
        fcgNUAudioBitrate->Visible = false;
        fcgNUAudioBitrate->Minimum = 0;
        fcgNUAudioBitrate->Maximum = 1536; //音声の最大レートは1536kbps
    }
    fcgCBAudio2pass->Enabled = astg->mode[index].enc_2pass != 0;
    if (!fcgCBAudio2pass->Enabled) fcgCBAudio2pass->Checked = false;
    SetfbcBTABEnable(fcgNUAudioBitrate->Visible, (int)fcgNUAudioBitrate->Maximum);

    bool delay_cut_available = astg->mode[index].delay > 0;
    fcgLBAudioDelayCut->Visible = delay_cut_available;
    fcgCXAudioDelayCut->Visible = delay_cut_available;
    if (delay_cut_available) {
        const bool delay_cut_edts_available = str_has_char(astg->cmd_raw) && str_has_char(sys_dat->exstg->s_mux[MUXER_MP4_RAW].delay_cmd);
        const int current_idx = fcgCXAudioDelayCut->SelectedIndex;
        const int items_to_set = _countof(AUDIO_DELAY_CUT_MODE) - 1 - ((delay_cut_edts_available) ? 0 : 1);
        fcgCXAudioDelayCut->BeginUpdate();
        fcgCXAudioDelayCut->Items->Clear();
        for (int i = 0; i < items_to_set; i++)
            fcgCXAudioDelayCut->Items->Add(String(AUDIO_DELAY_CUT_MODE[i]).ToString());
        fcgCXAudioDelayCut->EndUpdate();
        fcgCXAudioDelayCut->SelectedIndex = (current_idx >= items_to_set) ? 0 : current_idx;
    } else {
        fcgCXAudioDelayCut->SelectedIndex = 0;
    }
}

///////////////   設定ファイル関連   //////////////////////
System::Void frmConfig::CheckTSItemsEnabled(CONF_GUIEX *current_conf) {
    bool selected = (CheckedStgMenuItem != nullptr);
    fcgTSBSave->Enabled = (selected && memcmp(cnf_stgSelected, current_conf, sizeof(CONF_GUIEX)));
    fcgTSBDelete->Enabled = selected;
}

System::Void frmConfig::UncheckAllDropDownItem(ToolStripItem^ mItem) {
    ToolStripDropDownItem^ DropDownItem = dynamic_cast<ToolStripDropDownItem^>(mItem);
    if (DropDownItem == nullptr)
        return;
    for (int i = 0; i < DropDownItem->DropDownItems->Count; i++) {
        UncheckAllDropDownItem(DropDownItem->DropDownItems[i]);
        ToolStripMenuItem^ item = dynamic_cast<ToolStripMenuItem^>(DropDownItem->DropDownItems[i]);
        if (item != nullptr)
            item->Checked = false;
    }
}

System::Void frmConfig::CheckTSSettingsDropDownItem(ToolStripMenuItem^ mItem) {
    UncheckAllDropDownItem(fcgTSSettings);
    CheckedStgMenuItem = mItem;
    fcgTSSettings->Text = (mItem == nullptr) ? L"プロファイル" : mItem->Text;
    if (mItem != nullptr)
        mItem->Checked = true;
    fcgTSBSave->Enabled = false;
    fcgTSBDelete->Enabled = (mItem != nullptr);
}

ToolStripMenuItem^ frmConfig::fcgTSSettingsSearchItem(String^ stgPath, ToolStripItem^ mItem) {
    if (stgPath == nullptr)
        return nullptr;
    ToolStripDropDownItem^ DropDownItem = dynamic_cast<ToolStripDropDownItem^>(mItem);
    if (DropDownItem == nullptr)
        return nullptr;
    for (int i = 0; i < DropDownItem->DropDownItems->Count; i++) {
        ToolStripMenuItem^ item = fcgTSSettingsSearchItem(stgPath, DropDownItem->DropDownItems[i]);
        if (item != nullptr)
            return item;
        item = dynamic_cast<ToolStripMenuItem^>(DropDownItem->DropDownItems[i]);
        if (item      != nullptr && 
            item->Tag != nullptr && 
            0 == String::Compare(item->Tag->ToString(), stgPath, true))
            return item;
    }
    return nullptr;
}

ToolStripMenuItem^ frmConfig::fcgTSSettingsSearchItem(String^ stgPath) {
    return fcgTSSettingsSearchItem((stgPath != nullptr && stgPath->Length > 0) ? Path::GetFullPath(stgPath) : nullptr, fcgTSSettings);
}

System::Void frmConfig::SaveToStgFile(String^ stgName) {
    size_t nameLen = CountStringBytes(stgName) + 1; 
    char *stg_name = (char *)malloc(nameLen);
    GetCHARfromString(stg_name, nameLen, stgName);
    init_CONF_GUIEX(cnf_stgSelected, FALSE);
    FrmToConf(cnf_stgSelected);
    String^ stgDir = Path::GetDirectoryName(stgName);
    if (!Directory::Exists(stgDir))
        Directory::CreateDirectory(stgDir);
    guiEx_config exCnf;
    int result = exCnf.save_guiex_conf(cnf_stgSelected, stg_name);
    free(stg_name);
    switch (result) {
        case CONF_ERROR_FILE_OPEN:
            MessageBox::Show(L"設定ファイルオープンに失敗しました。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
            return;
        case CONF_ERROR_INVALID_FILENAME:
            MessageBox::Show(L"ファイル名に使用できない文字が含まれています。\n保存できません。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
            return;
        default:
            break;
    }
    init_CONF_GUIEX(cnf_stgSelected, FALSE);
    FrmToConf(cnf_stgSelected);
}

System::Void frmConfig::fcgTSBSave_Click(System::Object^  sender, System::EventArgs^  e) {
    if (CheckedStgMenuItem != nullptr)
        SaveToStgFile(CheckedStgMenuItem->Tag->ToString());
    CheckTSSettingsDropDownItem(CheckedStgMenuItem);
}

System::Void frmConfig::fcgTSBSaveNew_Click(System::Object^  sender, System::EventArgs^  e) {
    frmSaveNewStg::Instance::get()->setStgDir(String(sys_dat->exstg->s_local.stg_dir).ToString());
    if (CheckedStgMenuItem != nullptr)
        frmSaveNewStg::Instance::get()->setFilename(CheckedStgMenuItem->Text);
    frmSaveNewStg::Instance::get()->ShowDialog();
    String^ stgName = frmSaveNewStg::Instance::get()->StgFileName;
    if (stgName != nullptr && stgName->Length)
        SaveToStgFile(stgName);
    RebuildStgFileDropDown(nullptr);
    CheckTSSettingsDropDownItem(fcgTSSettingsSearchItem(stgName));
}

System::Void frmConfig::DeleteStgFile(ToolStripMenuItem^ mItem) {
    if (System::Windows::Forms::DialogResult::OK ==
        MessageBox::Show(L"設定ファイル " + mItem->Text + L" を削除してよろしいですか?",
        L"エラー", MessageBoxButtons::OKCancel, MessageBoxIcon::Exclamation)) 
    {
        File::Delete(mItem->Tag->ToString());
        RebuildStgFileDropDown(nullptr);
        CheckTSSettingsDropDownItem(nullptr);
        SetfcgTSLSettingsNotes(L"");
    }
}

System::Void frmConfig::fcgTSBDelete_Click(System::Object^  sender, System::EventArgs^  e) {
    DeleteStgFile(CheckedStgMenuItem);
}

System::Void frmConfig::fcgTSSettings_DropDownItemClicked(System::Object^  sender, System::Windows::Forms::ToolStripItemClickedEventArgs^  e) {
    ToolStripMenuItem^ ClickedMenuItem = dynamic_cast<ToolStripMenuItem^>(e->ClickedItem);
    if (ClickedMenuItem == nullptr)
        return;
    if (ClickedMenuItem->Tag == nullptr || ClickedMenuItem->Tag->ToString()->Length == 0)
        return;
    CONF_GUIEX load_stg;
    guiEx_config exCnf;
    char stg_path[MAX_PATH_LEN];
    GetCHARfromString(stg_path, sizeof(stg_path), ClickedMenuItem->Tag->ToString());
    if (exCnf.load_guiex_conf(&load_stg, stg_path) == CONF_ERROR_FILE_OPEN) {
        if (MessageBox::Show(L"設定ファイルオープンに失敗しました。\n"
                           + L"このファイルを削除しますか?",
                           L"エラー", MessageBoxButtons::YesNo, MessageBoxIcon::Error)
                           == System::Windows::Forms::DialogResult::Yes)
            DeleteStgFile(ClickedMenuItem);
        return;
    }
    ConfToFrm(&load_stg);
    CheckTSSettingsDropDownItem(ClickedMenuItem);
    memcpy(cnf_stgSelected, &load_stg, sizeof(CONF_GUIEX));
}

System::Void frmConfig::RebuildStgFileDropDown(ToolStripDropDownItem^ TS, String^ dir) {
    array<String^>^ subDirs = Directory::GetDirectories(dir);
    for (int i = 0; i < subDirs->Length; i++) {
        ToolStripMenuItem^ DDItem = gcnew ToolStripMenuItem(L"[ " + subDirs[i]->Substring(dir->Length+1) + L" ]");
        DDItem->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(this, &frmConfig::fcgTSSettings_DropDownItemClicked);
        DDItem->ForeColor = Color::Blue;
        DDItem->Tag = nullptr;
        RebuildStgFileDropDown(DDItem, subDirs[i]);
        TS->DropDownItems->Add(DDItem);
    }
    array<String^>^ stgList = Directory::GetFiles(dir, L"*.stg");
    for (int i = 0; i < stgList->Length; i++) {
        ToolStripMenuItem^ mItem = gcnew ToolStripMenuItem(Path::GetFileNameWithoutExtension(stgList[i]));
        mItem->Tag = stgList[i];
        TS->DropDownItems->Add(mItem);
    }
}

System::Void frmConfig::RebuildStgFileDropDown(String^ stgDir) {
    fcgTSSettings->DropDownItems->Clear();
    if (stgDir != nullptr)
        CurrentStgDir = stgDir;
    if (!Directory::Exists(CurrentStgDir))
        Directory::CreateDirectory(CurrentStgDir);
    RebuildStgFileDropDown(fcgTSSettings, Path::GetFullPath(CurrentStgDir));
}

//////////////   初期化関連     ////////////////
System::Void frmConfig::InitData(CONF_GUIEX *set_config, const SYSTEM_DATA *system_data) {
    if (set_config->size_all != CONF_INITIALIZED) {
        //初期化されていなければ初期化する
        init_CONF_GUIEX(set_config, FALSE);
    }
    conf = set_config;
    sys_dat = system_data;
}

System::Void frmConfig::InitComboBox() {
    //コンボボックスに値を設定する
    setComboBox(fcgCXEncCodec,          list_nvenc_codecs);
    setComboBox(fcgCXEncMode,           list_nvenc_rc_method);
    setComboBox(fcgCXCodecLevel,        list_avc_level);
    setComboBox(fcgCXCodecProfile,      h264_profile_names);
    setComboBox(fcgCXInterlaced,        list_interlaced);
    setComboBox(fcgCXAspectRatio,       list_aspect_ratio);
    setComboBox(fcgCXAdaptiveTransform, list_adapt_transform);
    setComboBox(fcgCXBDirectMode,       list_bdirect);
    setComboBox(fcgCXMVPrecision,       list_mv_presicion);
    setComboBox(fcgCXQualityPreset,     list_nvenc_preset_names);
    setComboBox(fcgCXVideoFormatH264,   list_videoformat);
    setComboBox(fcgCXTransferH264,      list_transfer);
    setComboBox(fcgCXColorMatrixH264,   list_colormatrix);
    setComboBox(fcgCXColorPrimH264,     list_colorprim);
    setComboBox(fcgCXVideoFormatHEVC,   list_videoformat);
    setComboBox(fcgCXTransferHEVC,      list_transfer);
    setComboBox(fcgCXColorMatrixHEVC,   list_colormatrix);
    setComboBox(fcgCXColorPrimHEVC,     list_colorprim);
    setComboBox(fcgCXHEVCTier,          h265_profile_names);
    setComboBox(fxgCXHEVCLevel,         list_hevc_level);
    setComboBox(fcgCXHEVCMaxCUSize,     list_hevc_cu_size);
    setComboBox(fcgCXHEVCMinCUSize,     list_hevc_cu_size);
    setComboBox(fcgCXHEVCOutBitDepth,   list_bitdepth);
    setComboBox(fcgCXAQ,                list_aq);
    setComboBox(fcgCXCudaSchdule,       list_cuda_schedule);
    setComboBox(fcgCXVppResizeAlg,      list_nppi_resize);
    setComboBox(fcgCXVppDenoiseMethod,  list_vpp_denoise);
    setComboBox(fcgCXVppDetailEnhance,  list_vpp_detail_enahance);
    setComboBox(fcgCXVppDebandSample,   list_vpp_deband);
    setComboBox(fcgCXVppAfsAnalyze,     list_vpp_afs_analyze);

    setComboBox(fcgCXAudioTempDir,  list_audtempdir);
    setComboBox(fcgCXMP4BoxTempDir, list_mp4boxtempdir);
    setComboBox(fcgCXTempDir,       list_tempdir);

    setComboBox(fcgCXAudioEncTiming, audio_enc_timing_desc);
    setComboBox(fcgCXAudioDelayCut,  AUDIO_DELAY_CUT_MODE);

    setMuxerCmdExNames(fcgCXMP4CmdEx, MUXER_MP4);
    setMuxerCmdExNames(fcgCXMKVCmdEx, MUXER_MKV);
#ifdef HIDE_MPEG2
    fcgCXMPGCmdEx->Items->Clear();
    fcgCXMPGCmdEx->Items->Add("");
#else
    setMuxerCmdExNames(fcgCXMPGCmdEx, MUXER_MPG);
#endif

    setAudioEncoderNames();

    setPriorityList(fcgCXMuxPriority);
    setPriorityList(fcgCXAudioPriority);
}

System::Void frmConfig::SetTXMaxLen(TextBox^ TX, int max_len) {
    TX->MaxLength = max_len;
    TX->Validating += gcnew System::ComponentModel::CancelEventHandler(this, &frmConfig::TX_LimitbyBytes);
}

System::Void frmConfig::SetTXMaxLenAll() {
    //MaxLengthに最大文字数をセットし、それをもとにバイト数計算を行うイベントをセットする。
    SetTXMaxLen(fcgTXAudioEncoderPath,   sizeof(sys_dat->exstg->s_aud[0].fullpath) - 1);
    SetTXMaxLen(fcgTXMP4MuxerPath,       sizeof(sys_dat->exstg->s_mux[MUXER_MP4].fullpath) - 1);
    SetTXMaxLen(fcgTXMKVMuxerPath,       sizeof(sys_dat->exstg->s_mux[MUXER_MKV].fullpath) - 1);
    SetTXMaxLen(fcgTXTC2MP4Path,         sizeof(sys_dat->exstg->s_mux[MUXER_TC2MP4].fullpath) - 1);
    SetTXMaxLen(fcgTXMPGMuxerPath,       sizeof(sys_dat->exstg->s_mux[MUXER_MPG].fullpath) - 1);
    SetTXMaxLen(fcgTXMP4RawPath,         sizeof(sys_dat->exstg->s_mux[MUXER_MP4_RAW].fullpath) - 1);
    SetTXMaxLen(fcgTXCustomTempDir,      sizeof(sys_dat->exstg->s_local.custom_tmp_dir) - 1);
    SetTXMaxLen(fcgTXCustomAudioTempDir, sizeof(sys_dat->exstg->s_local.custom_audio_tmp_dir) - 1);
    SetTXMaxLen(fcgTXMP4BoxTempDir,      sizeof(sys_dat->exstg->s_local.custom_mp4box_tmp_dir) - 1);
    SetTXMaxLen(fcgTXBatBeforeAudioPath, sizeof(conf->oth.batfile.before_audio) - 1);
    SetTXMaxLen(fcgTXBatAfterAudioPath,  sizeof(conf->oth.batfile.after_audio) - 1);
    SetTXMaxLen(fcgTXBatBeforePath,      sizeof(conf->oth.batfile.before_process) - 1);
    SetTXMaxLen(fcgTXBatAfterPath,       sizeof(conf->oth.batfile.after_process) - 1);
    fcgTSTSettingsNotes->MaxLength     = sizeof(conf->oth.notes) - 1;
}

System::Void frmConfig::InitStgFileList() {
    RebuildStgFileDropDown(String(sys_dat->exstg->s_local.stg_dir).ToString());
    stgChanged = false;
    CheckTSSettingsDropDownItem(nullptr);
}

System::Void frmConfig::fcgChangeEnabled(System::Object^  sender, System::EventArgs^  e) {
    bool h264_mode = fcgCXEncCodec->SelectedIndex == NV_ENC_H264;
    bool hevc_mode = fcgCXEncCodec->SelectedIndex == NV_ENC_HEVC;

    int vce_rc_method = list_nvenc_rc_method[fcgCXEncMode->SelectedIndex].value;
    bool cqp_mode = (vce_rc_method == NV_ENC_PARAMS_RC_CONSTQP);
    bool cbr_vbr_mode = (vce_rc_method == NV_ENC_PARAMS_RC_CBR || vce_rc_method == NV_ENC_PARAMS_RC_CBR_HQ || vce_rc_method == NV_ENC_PARAMS_RC_VBR || vce_rc_method == NV_ENC_PARAMS_RC_VBR_HQ);

    this->SuspendLayout();
    
    fcgPNHEVC->Visible = hevc_mode;
    fcgPNH264->Visible = h264_mode;
    fcgPNHEVCDetail->Visible = hevc_mode;
    fcgPNH264Detail->Visible = h264_mode;

    fcgPNQP->Visible = cqp_mode;
    fcgNUQPI->Enabled = cqp_mode;
    fcgNUQPP->Enabled = cqp_mode;
    fcgNUQPB->Enabled = cqp_mode;
    fcgLBQPI->Enabled = cqp_mode;
    fcgLBQPP->Enabled = cqp_mode;
    fcgLBQPB->Enabled = cqp_mode;

    fcgPNBitrate->Visible = !cqp_mode;
    fcgNUBitrate->Enabled = !cqp_mode;
    fcgLBBitrate->Enabled = !cqp_mode;
    fcgNUMaxkbps->Enabled = cbr_vbr_mode;
    fcgLBMaxkbps->Enabled = cbr_vbr_mode;
    fcgLBMaxBitrate2->Enabled = cbr_vbr_mode;

    fcggroupBoxResize->Enabled = fcgCBVppResize->Checked;
    fcgPNVppDenoiseKnn->Visible = (fcgCXVppDenoiseMethod->SelectedIndex == get_cx_index(list_vpp_denoise, _T("knn")));
    fcgPNVppDenoisePmd->Visible = (fcgCXVppDenoiseMethod->SelectedIndex == get_cx_index(list_vpp_denoise, _T("pmd")));
    fcgPNVppUnsharp->Visible    = (fcgCXVppDetailEnhance->SelectedIndex == get_cx_index(list_vpp_detail_enahance, _T("unsharp")));
    fcgPNVppEdgelevel->Visible  = (fcgCXVppDetailEnhance->SelectedIndex == get_cx_index(list_vpp_detail_enahance, _T("edgelevel")));
    fcggroupBoxVppDeband->Enabled = fcgCBVppDebandEnable->Checked;
    fcggroupBoxVppAfs->Enabled = fcCBVppAfsEnable->Checked;

    this->ResumeLayout();
    this->PerformLayout();
}

System::Void frmConfig::fcgCodecChanged(System::Object^  sender, System::EventArgs^  e) {
    bool h264_mode = fcgCXEncCodec->SelectedIndex == NV_ENC_H264;
    bool hevc_mode = fcgCXEncCodec->SelectedIndex == NV_ENC_HEVC;
    if (hevc_mode && !nvfeature_HEVCAvailable(featureCache)) {
        h264_mode = true;
        hevc_mode = false;
        fcgCXEncCodec->SelectedIndex = NV_ENC_H264;
    }
}

System::Void frmConfig::fcgChangeMuxerVisible(System::Object^  sender, System::EventArgs^  e) {
    //tc2mp4のチェック
    const bool enable_tc2mp4_muxer = (0 != str_has_char(sys_dat->exstg->s_mux[MUXER_TC2MP4].base_cmd));
    fcgTXTC2MP4Path->Visible = enable_tc2mp4_muxer;
    fcgLBTC2MP4Path->Visible = enable_tc2mp4_muxer;
    fcgBTTC2MP4Path->Visible = enable_tc2mp4_muxer;
    //mp4 rawのチェック
    const bool enable_mp4raw_muxer = (0 != str_has_char(sys_dat->exstg->s_mux[MUXER_MP4_RAW].base_cmd));
    fcgTXMP4RawPath->Visible = enable_mp4raw_muxer;
    fcgLBMP4RawPath->Visible = enable_mp4raw_muxer;
    fcgBTMP4RawPath->Visible = enable_mp4raw_muxer;
    //一時フォルダのチェック
    const bool enable_mp4_tmp = (0 != str_has_char(sys_dat->exstg->s_mux[MUXER_MP4].tmp_cmd));
    fcgCXMP4BoxTempDir->Visible = enable_mp4_tmp;
    fcgLBMP4BoxTempDir->Visible = enable_mp4_tmp;
    fcgTXMP4BoxTempDir->Visible = enable_mp4_tmp;
    fcgBTMP4BoxTempDir->Visible = enable_mp4_tmp;
    //Apple Chapterのチェック
    bool enable_mp4_apple_cmdex = false;
    for (int i = 0; i < sys_dat->exstg->s_mux[MUXER_MP4].ex_count; i++)
        enable_mp4_apple_cmdex |= (0 != str_has_char(sys_dat->exstg->s_mux[MUXER_MP4].ex_cmd[i].cmd_apple));
    fcgCBMP4MuxApple->Visible = enable_mp4_apple_cmdex;

    //位置の調整
    static const int HEIGHT = 31;
    fcgLBTC2MP4Path->Location = Point(fcgLBTC2MP4Path->Location.X, fcgLBMP4MuxerPath->Location.Y + HEIGHT * enable_tc2mp4_muxer);
    fcgTXTC2MP4Path->Location = Point(fcgTXTC2MP4Path->Location.X, fcgTXMP4MuxerPath->Location.Y + HEIGHT * enable_tc2mp4_muxer);
    fcgBTTC2MP4Path->Location = Point(fcgBTTC2MP4Path->Location.X, fcgBTMP4MuxerPath->Location.Y + HEIGHT * enable_tc2mp4_muxer);
    fcgLBMP4RawPath->Location = Point(fcgLBMP4RawPath->Location.X, fcgLBTC2MP4Path->Location.Y   + HEIGHT * enable_mp4raw_muxer);
    fcgTXMP4RawPath->Location = Point(fcgTXMP4RawPath->Location.X, fcgTXTC2MP4Path->Location.Y   + HEIGHT * enable_mp4raw_muxer);
    fcgBTMP4RawPath->Location = Point(fcgBTMP4RawPath->Location.X, fcgBTTC2MP4Path->Location.Y   + HEIGHT * enable_mp4raw_muxer);
}

System::Void frmConfig::SetStgEscKey(bool Enable) {
    if (this->KeyPreview == Enable)
        return;
    this->KeyPreview = Enable;
    if (Enable)
        this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &frmConfig::frmConfig_KeyDown);
    else
        this->KeyDown -= gcnew System::Windows::Forms::KeyEventHandler(this, &frmConfig::frmConfig_KeyDown);
}

System::Void frmConfig::AdjustLocation() {
    //デスクトップ領域(タスクバー等除く)
    System::Drawing::Rectangle screen = System::Windows::Forms::Screen::GetWorkingArea(this);
    //現在のデスクトップ領域の座標
    Point CurrentDesktopLocation = this->DesktopLocation::get();
    //チェック開始
    bool ChangeLocation = false;
    if (CurrentDesktopLocation.X + this->Size.Width > screen.Width) {
        ChangeLocation = true;
        CurrentDesktopLocation.X = clamp(screen.X - this->Size.Width, 4, CurrentDesktopLocation.X);
    }
    if (CurrentDesktopLocation.Y + this->Size.Height > screen.Height) {
        ChangeLocation = true;
        CurrentDesktopLocation.Y = clamp(screen.Y - this->Size.Height, 4, CurrentDesktopLocation.Y);
    }
    if (ChangeLocation) {
        this->StartPosition = FormStartPosition::Manual;
        this->DesktopLocation::set(CurrentDesktopLocation);
    }
}

std::list<NVGPUInfo> get_gpu_list();

System::Void frmConfig::InitForm() {
    fcgCXDevice->Items->Clear();
    fcgCXDevice->Items->Add(String(_T("Auto")).ToString());
    for (auto& gpu : get_gpu_list()) {
        fcgCXDevice->Items->Add(String(gpu.name.c_str()).ToString());
    }
    //CPU情報の取得
    SetupFeatureTable();
    getEnvironmentInfoDelegate = gcnew SetEnvironmentInfoDelegate(this, &frmConfig::SetEnvironmentInfo);
    getEnvironmentInfoDelegate->BeginInvoke(nullptr, nullptr);
    //ローカル設定のロード
    LoadLocalStg();
    //ローカル設定の反映
    SetLocalStg();
    //設定ファイル集の初期化
    InitStgFileList();
    //コンボボックスの値を設定
    InitComboBox();
    //タイトル表示
    this->Text = String(AUO_FULL_NAME).ToString();
    //バージョン情報,コンパイル日時
    fcgLBVersion->Text     = String(AUO_VERSION_NAME).ToString();
    fcgLBVersionDate->Text = L"build " + String(__DATE__).ToString() + L" " + String(__TIME__).ToString();
    //ツールチップ
    SetHelpToolTips();
    //パラメータセット
    ConfToFrm(conf);
    //イベントセット
    SetTXMaxLenAll(); //テキストボックスの最大文字数
    SetAllCheckChangedEvents(this); //変更の確認,ついでにNUのEnterEvent
    //フォームの変更可不可を更新
    fcgChangeMuxerVisible(nullptr, nullptr);
    fcgChangeEnabled(nullptr, nullptr);
    EnableSettingsNoteChange(false);
#ifdef HIDE_MPEG2
    tabPageMpgMux = fcgtabControlMux->TabPages[2];
    fcgtabControlMux->TabPages->RemoveAt(2);
#endif
    //表示位置の調整
    AdjustLocation();
    //キー設定
    SetStgEscKey(sys_dat->exstg->s_local.enable_stg_esc_key != 0);
    //フォントの設定
    if (str_has_char(sys_dat->exstg->s_local.conf_font.name))
        SetFontFamilyToForm(this, gcnew FontFamily(String(sys_dat->exstg->s_local.conf_font.name).ToString()), this->Font->FontFamily);
    //NVEncが実行できるか
    fcgPBNVEncLogoEnabled->Visible = featureCache && 0 < nvfeature_GetCachedNVEncCapability(featureCache).size();
    //HEVCエンコができるか
    if (!nvfeature_HEVCAvailable(featureCache)) {
        fcgCXEncCodec->Items[NV_ENC_HEVC] = L"-------------";
    }
    fcgCXEncCodec->SelectedIndexChanged += gcnew System::EventHandler(this, &frmConfig::fcgCodecChanged);
    fcgCodecChanged(nullptr, nullptr);
}

/////////////         データ <-> GUI     /////////////
System::Void frmConfig::ConfToFrm(CONF_GUIEX *cnf) {
    this->SuspendLayout();

    SetCXIndex(fcgCXEncCodec,          get_index_from_value(cnf->nvenc.codec, list_nvenc_codecs));
    SetCXIndex(fcgCXEncMode,           get_cx_index(list_nvenc_rc_method, cnf->nvenc.enc_config.rcParams.rateControlMode));
    SetNUValue(fcgNUBitrate,           cnf->nvenc.enc_config.rcParams.averageBitRate / 1000);
    SetNUValue(fcgNUMaxkbps,           cnf->nvenc.enc_config.rcParams.maxBitRate / 1000);
    SetNUValue(fcgNUQPI,               cnf->nvenc.enc_config.rcParams.constQP.qpIntra);
    SetNUValue(fcgNUQPP,               cnf->nvenc.enc_config.rcParams.constQP.qpInterP);
    SetNUValue(fcgNUQPB,               cnf->nvenc.enc_config.rcParams.constQP.qpInterB);
    SetNUValue(fcgNUVBRTragetQuality,  Decimal(cnf->nvenc.enc_config.rcParams.targetQuality + cnf->nvenc.enc_config.rcParams.targetQualityLSB / 256.0));
    SetNUValue(fcgNUGopLength,         cnf->nvenc.enc_config.gopLength);
    SetNUValue(fcgNUBframes,           cnf->nvenc.enc_config.frameIntervalP - 1);
    SetCXIndex(fcgCXQualityPreset,     get_index_from_value(cnf->nvenc.preset, list_nvenc_preset_names));
    SetCXIndex(fcgCXMVPrecision,       get_cx_index(list_mv_presicion, cnf->nvenc.enc_config.mvPrecision));
    SetNUValue(fcgNUVBVBufsize, cnf->nvenc.enc_config.rcParams.vbvBufferSize / 1000);
    SetNUValue(fcgNULookaheadDepth,   (cnf->nvenc.enc_config.rcParams.enableLookahead) ? cnf->nvenc.enc_config.rcParams.lookaheadDepth : 0);
    uint32_t nAQ = 0;
    nAQ |= cnf->nvenc.enc_config.rcParams.enableAQ ? NV_ENC_AQ_SPATIAL : 0x00;
    nAQ |= cnf->nvenc.enc_config.rcParams.enableTemporalAQ ? NV_ENC_AQ_TEMPORAL : 0x00;
    SetCXIndex(fcgCXAQ,                get_cx_index(list_aq, nAQ));
    SetNUValue(fcgNUAQStrength,        cnf->nvenc.enc_config.rcParams.aqStrength);
    fcgCBWeightP->Checked            = cnf->nvenc.weightp != 0;
    
    if (cnf->nvenc.par[0] * cnf->nvenc.par[1] <= 0)
        cnf->nvenc.par[0] = cnf->nvenc.par[1] = 0;
    SetCXIndex(fcgCXAspectRatio, (cnf->nvenc.par[0] < 0));
    SetNUValue(fcgNUAspectRatioX, abs(cnf->nvenc.par[0]));
    SetNUValue(fcgNUAspectRatioY, abs(cnf->nvenc.par[1]));

    SetCXIndex(fcgCXDevice,      cnf->nvenc.deviceID+1); //先頭は"Auto"なので+1
    SetCXIndex(fcgCXCudaSchdule, cnf->nvenc.cuda_schedule);
    fcgCBPerfMonitor->Checked = 0 != cnf->nvenc.perf_monitor;

    //QPDetail
    fcgCBQPMin->Checked = cnf->nvenc.enc_config.rcParams.enableMinQP != 0;
    fcgCBQPMax->Checked = cnf->nvenc.enc_config.rcParams.enableMaxQP != 0;
    fcgCBQPInit->Checked = cnf->nvenc.enc_config.rcParams.enableInitialRCQP != 0;
    SetNUValue(fcgNUQPMinI, cnf->nvenc.enc_config.rcParams.minQP.qpIntra);
    SetNUValue(fcgNUQPMinP, cnf->nvenc.enc_config.rcParams.minQP.qpInterP);
    SetNUValue(fcgNUQPMinB, cnf->nvenc.enc_config.rcParams.minQP.qpInterB);
    SetNUValue(fcgNUQPMaxI, cnf->nvenc.enc_config.rcParams.maxQP.qpIntra);
    SetNUValue(fcgNUQPMaxP, cnf->nvenc.enc_config.rcParams.maxQP.qpInterP);
    SetNUValue(fcgNUQPMaxB, cnf->nvenc.enc_config.rcParams.maxQP.qpInterB);
    SetNUValue(fcgNUQPInitI, cnf->nvenc.enc_config.rcParams.initialRCQP.qpIntra);
    SetNUValue(fcgNUQPInitP, cnf->nvenc.enc_config.rcParams.initialRCQP.qpInterP);
    SetNUValue(fcgNUQPInitB, cnf->nvenc.enc_config.rcParams.initialRCQP.qpInterB);

    //H.264
    SetNUValue(fcgNURefFrames,         cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.maxNumRefFrames);
    SetCXIndex(fcgCXInterlaced,   get_cx_index(list_interlaced, picstruct_enc_to_rgy(cnf->nvenc.pic_struct)));
    SetCXIndex(fcgCXCodecLevel,   get_cx_index(list_avc_level,            cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.level));
    SetCXIndex(fcgCXCodecProfile, get_index_from_value(get_value_from_guid(cnf->nvenc.enc_config.profileGUID, h264_profile_names), h264_profile_names));
    SetNUValue(fcgNUSlices,       cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.sliceModeData);
    fcgCBBluray->Checked       = 0 != cnf->nvenc.bluray;
    fcgCBCABAC->Checked        = NV_ENC_H264_ENTROPY_CODING_MODE_CAVLC != cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.entropyCodingMode;
    fcgCBDeblock->Checked                                          = 0 == cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.disableDeblockingFilterIDC;
    SetCXIndex(fcgCXVideoFormatH264,       get_cx_index(list_videoformat, cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.videoFormat));
    fcgCBFullrangeH264->Checked                                    = 0 != cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.videoFullRangeFlag;
    SetCXIndex(fcgCXTransferH264,          get_cx_index(list_transfer,    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.transferCharacteristics));
    SetCXIndex(fcgCXColorMatrixH264,       get_cx_index(list_colormatrix, cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.colourMatrix));
    SetCXIndex(fcgCXColorPrimH264,         get_cx_index(list_colorprim,   cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.colourPrimaries));
    SetCXIndex(fcgCXAdaptiveTransform, get_cx_index(list_adapt_transform, cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.adaptiveTransformMode));
    SetCXIndex(fcgCXBDirectMode,       get_cx_index(list_bdirect,         cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.bdirectMode));
    
    //HEVC
    SetCXIndex(fcgCXHEVCOutBitDepth,       get_cx_index(list_bitdepth, cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.pixelBitDepthMinus8));
    SetCXIndex(fcgCXHEVCTier,      get_index_from_value(cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.tier, h265_profile_names));
    SetCXIndex(fxgCXHEVCLevel,     get_cx_index(list_hevc_level,   cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.level));
    SetCXIndex(fcgCXHEVCMaxCUSize, get_cx_index(list_hevc_cu_size, cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.maxCUSize));
    SetCXIndex(fcgCXHEVCMinCUSize, get_cx_index(list_hevc_cu_size, cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.minCUSize));
    SetCXIndex(fcgCXVideoFormatHEVC,       get_cx_index(list_videoformat, cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.videoFormat));
    fcgCBFullrangeHEVC->Checked                                    = 0 != cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.videoFullRangeFlag;
    SetCXIndex(fcgCXTransferHEVC,          get_cx_index(list_transfer,    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.transferCharacteristics));
    SetCXIndex(fcgCXColorMatrixHEVC,       get_cx_index(list_colormatrix, cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.colourMatrix));
    SetCXIndex(fcgCXColorPrimHEVC,         get_cx_index(list_colorprim,   cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.colourPrimaries));

    //fcgCBShowPerformanceInfo->Checked = (CHECK_PERFORMANCE) ? cnf->vid.vce_ext_prm.show_performance_info != 0 : false;

        //SetCXIndex(fcgCXX264Priority,        cnf->vid.priority);
        SetCXIndex(fcgCXTempDir,             cnf->oth.temp_dir);
        fcgCBAFS->Checked                  = cnf->vid.afs != 0; 
        fcgCBAuoTcfileout->Checked         = cnf->vid.auo_tcfile_out != 0;

        fcgCBVppPerfMonitor->Checked   = cnf->vpp.vpp_perf_monitor != 0;
        fcgCBVppResize->Checked        = cnf->vpp.resize_enable != 0;
        SetNUValue(fcgNUVppResizeWidth,  cnf->vpp.resize_width);
        SetNUValue(fcgNUVppResizeHeight, cnf->vpp.resize_height);
        SetCXIndex(fcgCXVppResizeAlg,    get_cx_index(list_nppi_resize, cnf->vpp.resize_interp));
        int dnoise_idx = 0;
        if (cnf->vpp.knn.enable) {
            dnoise_idx = get_cx_index(list_vpp_denoise, _T("knn"));
        } else if (cnf->vpp.pmd.enable) {
            dnoise_idx = get_cx_index(list_vpp_denoise, _T("pmd"));
        }
        SetCXIndex(fcgCXVppDenoiseMethod,        dnoise_idx);

        int detail_enahance_idx = 0;
        if (cnf->vpp.unsharp.enable) {
            detail_enahance_idx = get_cx_index(list_vpp_detail_enahance, _T("unsharp"));
        } else if (cnf->vpp.edgelevel.enable) {
            detail_enahance_idx = get_cx_index(list_vpp_detail_enahance, _T("edgelevel"));
        }
        SetCXIndex(fcgCXVppDetailEnhance, detail_enahance_idx);

        SetNUValue(fcgNUVppDenoiseKnnRadius,     cnf->vpp.knn.radius);
        SetNUValue(fcgNUVppDenoiseKnnStrength,   cnf->vpp.knn.strength);
        SetNUValue(fcgNUVppDenoiseKnnThreshold,  cnf->vpp.knn.lerp_threshold);
        SetNUValue(fcgNUVppDenoisePmdApplyCount, cnf->vpp.pmd.applyCount);
        SetNUValue(fcgNUVppDenoisePmdStrength,   cnf->vpp.pmd.strength);
        SetNUValue(fcgNUVppDenoisePmdThreshold,  cnf->vpp.pmd.threshold);
        fcgCBVppDebandEnable->Checked          = cnf->vpp.deband.enable;
        SetNUValue(fcgNUVppDebandRange,          cnf->vpp.deband.range);
        SetNUValue(fcgNUVppDebandThreY,          cnf->vpp.deband.threY);
        SetNUValue(fcgNUVppDebandThreCb,         cnf->vpp.deband.threCb);
        SetNUValue(fcgNUVppDebandThreCr,         cnf->vpp.deband.threCr);
        SetNUValue(fcgNUVppDebandDitherY,        cnf->vpp.deband.ditherY);
        SetNUValue(fcgNUVppDebandDitherC,        cnf->vpp.deband.ditherC);
        SetCXIndex(fcgCXVppDebandSample,         cnf->vpp.deband.sample);
        fcgCBVppDebandBlurFirst->Checked       = cnf->vpp.deband.blurFirst;
        fcgCBVppDebandRandEachFrame->Checked   = cnf->vpp.deband.randEachFrame;
        SetNUValue(fcgNUVppUnsharpRadius,        cnf->vpp.unsharp.radius);
        SetNUValue(fcgNUVppUnsharpWeight,        cnf->vpp.unsharp.weight);
        SetNUValue(fcgNUVppUnsharpThreshold,     cnf->vpp.unsharp.threshold);
        SetNUValue(fcgNUVppEdgelevelStrength,    cnf->vpp.edgelevel.strength);
        SetNUValue(fcgNUVppEdgelevelThreshold,   cnf->vpp.edgelevel.threshold);
        SetNUValue(fcgNUVppEdgelevelBlack,       cnf->vpp.edgelevel.black);
        SetNUValue(fcgNUVppEdgelevelWhite,       cnf->vpp.edgelevel.white);
        SetNUValue(fcgNUVppAfsUp,                cnf->vpp.afs.clip.top);
        SetNUValue(fcgNUVppAfsBottom,            cnf->vpp.afs.clip.bottom);
        SetNUValue(fcgNUVppAfsLeft,              cnf->vpp.afs.clip.left);
        SetNUValue(fcgNUVppAfsRight,             cnf->vpp.afs.clip.right);
        SetNUValue(fcgNUVppAfsMethodSwitch,      cnf->vpp.afs.method_switch);
        SetNUValue(fcgNUVppAfsCoeffShift,        cnf->vpp.afs.coeff_shift);
        SetNUValue(fcgNUVppAfsThreShift,         cnf->vpp.afs.thre_shift);
        SetNUValue(fcgNUVppAfsThreDeint,         cnf->vpp.afs.thre_deint);
        SetNUValue(fcgNUVppAfsThreYMotion,       cnf->vpp.afs.thre_Ymotion);
        SetNUValue(fcgNUVppAfsThreCMotion,       cnf->vpp.afs.thre_Cmotion);
        SetCXIndex(fcgCXVppAfsAnalyze,           cnf->vpp.afs.analyze);
        fcCBVppAfsEnable->Checked              = cnf->vpp.afs.enable != 0;
        fcgCBVppAfsShift->Checked              = cnf->vpp.afs.shift != 0;
        fcgCBVppAfsDrop->Checked               = cnf->vpp.afs.drop != 0;
        fcgCBVppAfsSmooth->Checked             = cnf->vpp.afs.smooth != 0;
        fcgCBVppAfs24fps->Checked              = cnf->vpp.afs.force24 != 0;
        fcgCBVppAfsTune->Checked               = cnf->vpp.afs.tune != 0;

        //音声
        fcgCBAudioOnly->Checked            = cnf->oth.out_audio_only != 0;
        fcgCBFAWCheck->Checked             = cnf->aud.faw_check != 0;
        SetCXIndex(fcgCXAudioEncoder,        cnf->aud.encoder);
        fcgCBAudio2pass->Checked           = cnf->aud.use_2pass != 0;
        fcgCBAudioUsePipe->Checked = (CurrentPipeEnabled && !cnf->aud.use_wav);
        SetCXIndex(fcgCXAudioDelayCut,       cnf->aud.delay_cut);
        SetCXIndex(fcgCXAudioEncMode,        cnf->aud.enc_mode);
        SetNUValue(fcgNUAudioBitrate,       (cnf->aud.bitrate != 0) ? cnf->aud.bitrate : GetCurrentAudioDefaultBitrate());
        SetCXIndex(fcgCXAudioPriority,       cnf->aud.priority);
        SetCXIndex(fcgCXAudioTempDir,        cnf->aud.aud_temp_dir);
        SetCXIndex(fcgCXAudioEncTiming,      cnf->aud.audio_encode_timing);
        fcgCBRunBatBeforeAudio->Checked    =(cnf->oth.run_bat & RUN_BAT_BEFORE_AUDIO) != 0;
        fcgCBRunBatAfterAudio->Checked     =(cnf->oth.run_bat & RUN_BAT_AFTER_AUDIO) != 0;
        fcgTXBatBeforeAudioPath->Text      = String(cnf->oth.batfile.before_audio).ToString();
        fcgTXBatAfterAudioPath->Text       = String(cnf->oth.batfile.after_audio).ToString();

        //mux
        fcgCBMP4MuxerExt->Checked          = cnf->mux.disable_mp4ext == 0;
        fcgCBMP4MuxApple->Checked          = cnf->mux.apple_mode != 0;
        SetCXIndex(fcgCXMP4CmdEx,            cnf->mux.mp4_mode);
        SetCXIndex(fcgCXMP4BoxTempDir,       cnf->mux.mp4_temp_dir);
        fcgCBMKVMuxerExt->Checked          = cnf->mux.disable_mkvext == 0;
        SetCXIndex(fcgCXMKVCmdEx,            cnf->mux.mkv_mode);
        fcgCBMPGMuxerExt->Checked          = cnf->mux.disable_mpgext == 0;
        SetCXIndex(fcgCXMPGCmdEx,            cnf->mux.mpg_mode);
        fcgCBMuxMinimize->Checked          = cnf->mux.minimized != 0;
        SetCXIndex(fcgCXMuxPriority,         cnf->mux.priority);

        fcgCBRunBatBefore->Checked         =(cnf->oth.run_bat & RUN_BAT_BEFORE_PROCESS) != 0;
        fcgCBRunBatAfter->Checked          =(cnf->oth.run_bat & RUN_BAT_AFTER_PROCESS)  != 0;
        fcgCBWaitForBatBefore->Checked     =(cnf->oth.dont_wait_bat_fin & RUN_BAT_BEFORE_PROCESS) == 0;
        fcgCBWaitForBatAfter->Checked      =(cnf->oth.dont_wait_bat_fin & RUN_BAT_AFTER_PROCESS)  == 0;
        fcgTXBatBeforePath->Text           = String(cnf->oth.batfile.before_process).ToString();
        fcgTXBatAfterPath->Text            = String(cnf->oth.batfile.after_process).ToString();

        SetfcgTSLSettingsNotes(cnf->oth.notes);

    this->ResumeLayout();
    this->PerformLayout();
}

System::Void frmConfig::FrmToConf(CONF_GUIEX *cnf) {
    //これもひたすら書くだけ。めんどい
    cnf->nvenc.codec = list_nvenc_codecs[fcgCXEncCodec->SelectedIndex].value;
    cnf->nvenc.enc_config.rcParams.rateControlMode = (NV_ENC_PARAMS_RC_MODE)list_nvenc_rc_method[fcgCXEncMode->SelectedIndex].value;
    cnf->nvenc.enc_config.rcParams.averageBitRate = (int)fcgNUBitrate->Value * 1000;
    cnf->nvenc.enc_config.rcParams.maxBitRate = (int)fcgNUMaxkbps->Value * 1000;
    cnf->nvenc.enc_config.rcParams.constQP.qpIntra  = (int)fcgNUQPI->Value;
    cnf->nvenc.enc_config.rcParams.constQP.qpInterP = (int)fcgNUQPP->Value;
    cnf->nvenc.enc_config.rcParams.constQP.qpInterB = (int)fcgNUQPB->Value;
    cnf->nvenc.enc_config.gopLength = (int)fcgNUGopLength->Value;
    const double targetQualityDouble = (double)fcgNUVBRTragetQuality->Value;
    const int targetQualityInt = (int)targetQualityDouble;
    cnf->nvenc.enc_config.rcParams.targetQuality = (uint8_t)targetQualityInt;
    cnf->nvenc.enc_config.rcParams.targetQualityLSB = (uint8_t)clamp(((targetQualityDouble - targetQualityInt) * 256.0), 0, 255);
    cnf->nvenc.enc_config.frameIntervalP = (int)fcgNUBframes->Value + 1;
    cnf->nvenc.enc_config.mvPrecision = (NV_ENC_MV_PRECISION)list_mv_presicion[fcgCXMVPrecision->SelectedIndex].value;
    cnf->nvenc.enc_config.rcParams.vbvBufferSize = (int)fcgNUVBVBufsize->Value * 1000;
    cnf->nvenc.preset = list_nvenc_preset_names[fcgCXQualityPreset->SelectedIndex].value;

    int nLookaheadDepth = (int)fcgNULookaheadDepth->Value;
    if (nLookaheadDepth) {
        cnf->nvenc.enc_config.rcParams.enableLookahead = 1;
        cnf->nvenc.enc_config.rcParams.lookaheadDepth = (uint16_t)clamp(nLookaheadDepth, 0, 32);
    } else {
        cnf->nvenc.enc_config.rcParams.enableLookahead = 0;
        cnf->nvenc.enc_config.rcParams.lookaheadDepth = 0;
    }
    uint32_t nAQ = list_aq[fcgCXAQ->SelectedIndex].value;
    cnf->nvenc.enc_config.rcParams.enableAQ         = (nAQ & NV_ENC_AQ_SPATIAL)  ? 1 : 0;
    cnf->nvenc.enc_config.rcParams.enableTemporalAQ = (nAQ & NV_ENC_AQ_TEMPORAL) ? 1 : 0;
    cnf->nvenc.enc_config.rcParams.aqStrength = (uint32_t)clamp(fcgNUAQStrength->Value, 0, 15);
    cnf->nvenc.weightp                        = (fcgCBWeightP->Checked) ? 1 : 0;


    cnf->nvenc.par[0]                = (int)fcgNUAspectRatioX->Value;
    cnf->nvenc.par[1]                = (int)fcgNUAspectRatioY->Value;
    if (fcgCXAspectRatio->SelectedIndex == 1) {
        cnf->nvenc.par[0] *= -1;
        cnf->nvenc.par[1] *= -1;
    }

    cnf->nvenc.deviceID = (int)fcgCXDevice->SelectedIndex-1; //先頭は"Auto"なので-1
    cnf->nvenc.cuda_schedule = list_cuda_schedule[fcgCXCudaSchdule->SelectedIndex].value;

    //QPDetail
    cnf->nvenc.enc_config.rcParams.enableMinQP = (fcgCBQPMin->Checked) ? 1 : 0;
    cnf->nvenc.enc_config.rcParams.enableMaxQP = (fcgCBQPMax->Checked) ? 1 : 0;
    cnf->nvenc.enc_config.rcParams.enableInitialRCQP = (fcgCBQPInit->Checked) ? 1 : 0;
    cnf->nvenc.enc_config.rcParams.minQP.qpIntra  = (int)fcgNUQPMinI->Value;
    cnf->nvenc.enc_config.rcParams.minQP.qpInterP = (int)fcgNUQPMinP->Value;
    cnf->nvenc.enc_config.rcParams.minQP.qpInterB = (int)fcgNUQPMinB->Value;
    cnf->nvenc.enc_config.rcParams.maxQP.qpIntra  = (int)fcgNUQPMaxI->Value;
    cnf->nvenc.enc_config.rcParams.maxQP.qpInterP = (int)fcgNUQPMaxP->Value;
    cnf->nvenc.enc_config.rcParams.maxQP.qpInterB = (int)fcgNUQPMaxB->Value;
    cnf->nvenc.enc_config.rcParams.initialRCQP.qpIntra  = (int)fcgNUQPInitI->Value;
    cnf->nvenc.enc_config.rcParams.initialRCQP.qpInterP = (int)fcgNUQPInitP->Value;
    cnf->nvenc.enc_config.rcParams.initialRCQP.qpInterB = (int)fcgNUQPInitB->Value;

    cnf->nvenc.perf_monitor = fcgCBPerfMonitor->Checked;

    //H.264
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.bdirectMode = (NV_ENC_H264_BDIRECT_MODE)list_bdirect[fcgCXBDirectMode->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.maxNumRefFrames = (int)fcgNURefFrames->Value;
    cnf->nvenc.pic_struct = picstruct_rgy_to_enc((RGY_PICSTRUCT)list_interlaced[fcgCXInterlaced->SelectedIndex].value);
    cnf->nvenc.enc_config.profileGUID = get_guid_from_value(h264_profile_names[fcgCXCodecProfile->SelectedIndex].value, h264_profile_names);
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.adaptiveTransformMode = (NV_ENC_H264_ADAPTIVE_TRANSFORM_MODE)list_adapt_transform[fcgCXAdaptiveTransform->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.level = list_avc_level[fcgCXCodecLevel->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.sliceModeData = (int)fcgNUSlices->Value;
    cnf->nvenc.bluray = fcgCBBluray->Checked;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.entropyCodingMode = (fcgCBCABAC->Checked) ? NV_ENC_H264_ENTROPY_CODING_MODE_CABAC : NV_ENC_H264_ENTROPY_CODING_MODE_CAVLC;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.disableDeblockingFilterIDC = false == fcgCBDeblock->Checked;

    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.videoFormat             = list_videoformat[fcgCXVideoFormatH264->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.videoFullRangeFlag      = fcgCBFullrangeH264->Checked;
    if (fcgCBFullrangeH264->Checked || fcgCXVideoFormatH264->SelectedIndex == get_cx_index(list_videoformat, "undef")) {
        cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.videoSignalTypePresentFlag = 1;
    }
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.transferCharacteristics = list_transfer[fcgCXTransferH264->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.colourMatrix            = list_colormatrix[fcgCXColorMatrixH264->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.colourPrimaries         = list_colorprim[fcgCXColorPrimH264->SelectedIndex].value;
    if (   fcgCXTransferH264->SelectedIndex    == get_cx_index(list_transfer,    "undef")
        || fcgCXColorMatrixH264->SelectedIndex == get_cx_index(list_colormatrix, "undef")
        || fcgCXColorPrimH264->SelectedIndex   == get_cx_index(list_colorprim,   "undef")) {
        cnf->nvenc.codecConfig[NV_ENC_H264].h264Config.h264VUIParameters.colourDescriptionPresentFlag = 1;
    }

    //HEVC

    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.pixelBitDepthMinus8 = list_bitdepth[fcgCXHEVCOutBitDepth->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.maxNumRefFramesInDPB = (int)fcgNURefFrames->Value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.level = list_hevc_level[fxgCXHEVCLevel->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.tier  = h265_profile_names[fcgCXHEVCTier->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.maxCUSize = (NV_ENC_HEVC_CUSIZE)list_hevc_cu_size[fcgCXHEVCMaxCUSize->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.minCUSize = (NV_ENC_HEVC_CUSIZE)list_hevc_cu_size[fcgCXHEVCMinCUSize->SelectedIndex].value;

    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.videoFormat             = list_videoformat[fcgCXVideoFormatHEVC->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.videoFullRangeFlag      = fcgCBFullrangeHEVC->Checked;
    if (fcgCBFullrangeH264->Checked || fcgCXVideoFormatH264->SelectedIndex == get_cx_index(list_videoformat, "undef")) {
        cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.videoSignalTypePresentFlag = 1;
    }
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.transferCharacteristics = list_transfer[fcgCXTransferHEVC->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.colourMatrix            = list_colormatrix[fcgCXColorMatrixHEVC->SelectedIndex].value;
    cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.colourPrimaries         = list_colorprim[fcgCXColorPrimHEVC->SelectedIndex].value;
    if (   fcgCXTransferHEVC->SelectedIndex    == get_cx_index(list_transfer,    "undef")
        || fcgCXColorMatrixHEVC->SelectedIndex == get_cx_index(list_colormatrix, "undef")
        || fcgCXColorPrimHEVC->SelectedIndex   == get_cx_index(list_colorprim,   "undef")) {
        cnf->nvenc.codecConfig[NV_ENC_HEVC].hevcConfig.hevcVUIParameters.colourDescriptionPresentFlag = 1;
    }

    //cnf->vid.vce_ext_prm.show_performance_info = fcgCBShowPerformanceInfo->Checked;

    cnf->vid.afs                    = FALSE;
    cnf->vid.auo_tcfile_out         = FALSE;
    /*cnf->qsv.nPAR[0]                = (int)fcgNUAspectRatioX->Value;
    cnf->qsv.nPAR[1]                = (int)fcgNUAspectRatioY->Value;
    if (fcgCXAspectRatio->SelectedIndex == 1) {
        cnf->qsv.nPAR[0] *= -1;
        cnf->qsv.nPAR[1] *= -1;
    }*/

    //拡張部
    cnf->oth.temp_dir               = fcgCXTempDir->SelectedIndex;
    cnf->vid.afs                    = fcgCBAFS->Checked;
    cnf->vid.auo_tcfile_out         = fcgCBAuoTcfileout->Checked;

    cnf->vpp.vpp_perf_monitor       = fcgCBVppPerfMonitor->Checked;

    cnf->vpp.resize_enable          = fcgCBVppResize->Checked;
    cnf->vpp.resize_width           = (int)fcgNUVppResizeWidth->Value;
    cnf->vpp.resize_height          = (int)fcgNUVppResizeHeight->Value;
    cnf->vpp.resize_interp          = list_nppi_resize[fcgCXVppResizeAlg->SelectedIndex].value;

    cnf->vpp.knn.enable             = fcgCXVppDenoiseMethod->SelectedIndex == get_cx_index(list_vpp_denoise, _T("knn"));
    cnf->vpp.knn.radius             = (int)fcgNUVppDenoiseKnnRadius->Value;
    cnf->vpp.knn.strength           = (float)fcgNUVppDenoiseKnnStrength->Value;
    cnf->vpp.knn.lerp_threshold     = (float)fcgNUVppDenoiseKnnThreshold->Value;

    cnf->vpp.pmd.enable             = fcgCXVppDenoiseMethod->SelectedIndex == get_cx_index(list_vpp_denoise, _T("pmd"));
    cnf->vpp.pmd.applyCount         = (int)fcgNUVppDenoisePmdApplyCount->Value;
    cnf->vpp.pmd.strength           = (float)fcgNUVppDenoisePmdStrength->Value;
    cnf->vpp.pmd.threshold          = (float)fcgNUVppDenoisePmdThreshold->Value;

    cnf->vpp.unsharp.enable         = fcgCXVppDetailEnhance->SelectedIndex == get_cx_index(list_vpp_detail_enahance, _T("unsharp"));
    cnf->vpp.unsharp.radius         = (int)fcgNUVppUnsharpRadius->Value;
    cnf->vpp.unsharp.weight         = (float)fcgNUVppUnsharpWeight->Value;
    cnf->vpp.unsharp.threshold      = (float)fcgNUVppUnsharpThreshold->Value;

    cnf->vpp.edgelevel.enable       = fcgCXVppDetailEnhance->SelectedIndex == get_cx_index(list_vpp_detail_enahance, _T("edgelevel"));
    cnf->vpp.edgelevel.strength     = (float)fcgNUVppEdgelevelStrength->Value;
    cnf->vpp.edgelevel.threshold    = (float)fcgNUVppEdgelevelThreshold->Value;
    cnf->vpp.edgelevel.black        = (float)fcgNUVppEdgelevelBlack->Value;
    cnf->vpp.edgelevel.white        = (float)fcgNUVppEdgelevelWhite->Value;

    cnf->vpp.deband.enable          = fcgCBVppDebandEnable->Checked;
    cnf->vpp.deband.range           = (int)fcgNUVppDebandRange->Value;
    cnf->vpp.deband.threY           = (int)fcgNUVppDebandThreY->Value;
    cnf->vpp.deband.threCb          = (int)fcgNUVppDebandThreCb->Value;
    cnf->vpp.deband.threCr          = (int)fcgNUVppDebandThreCr->Value;
    cnf->vpp.deband.ditherY         = (int)fcgNUVppDebandDitherY->Value;
    cnf->vpp.deband.ditherC         = (int)fcgNUVppDebandDitherC->Value;
    cnf->vpp.deband.sample            = fcgCXVppDebandSample->SelectedIndex;
    cnf->vpp.deband.blurFirst       = fcgCBVppDebandBlurFirst->Checked;
    cnf->vpp.deband.randEachFrame   = fcgCBVppDebandRandEachFrame->Checked;

    cnf->vpp.afs.enable             = fcCBVppAfsEnable->Checked;
    cnf->vpp.afs.clip.top           = (int)fcgNUVppAfsUp->Value;
    cnf->vpp.afs.clip.bottom        = (int)fcgNUVppAfsBottom->Value;
    cnf->vpp.afs.clip.left          = (int)fcgNUVppAfsLeft->Value;
    cnf->vpp.afs.clip.right         = (int)fcgNUVppAfsRight->Value;
    cnf->vpp.afs.method_switch      = (int)fcgNUVppAfsMethodSwitch->Value;
    cnf->vpp.afs.coeff_shift        = (int)fcgNUVppAfsCoeffShift->Value;
    cnf->vpp.afs.thre_shift         = (int)fcgNUVppAfsThreShift->Value;
    cnf->vpp.afs.thre_deint         = (int)fcgNUVppAfsThreDeint->Value;
    cnf->vpp.afs.thre_Ymotion       = (int)fcgNUVppAfsThreYMotion->Value;
    cnf->vpp.afs.thre_Cmotion       = (int)fcgNUVppAfsThreCMotion->Value;
    cnf->vpp.afs.analyze            = fcgCXVppAfsAnalyze->SelectedIndex;
    cnf->vpp.afs.shift              = fcgCBVppAfsShift->Checked;
    cnf->vpp.afs.drop               = fcgCBVppAfsDrop->Checked;
    cnf->vpp.afs.smooth             = fcgCBVppAfsSmooth->Checked;
    cnf->vpp.afs.force24            = fcgCBVppAfs24fps->Checked;
    cnf->vpp.afs.tune               = fcgCBVppAfsTune->Checked;

    //音声部
    cnf->aud.encoder                = fcgCXAudioEncoder->SelectedIndex;
    cnf->oth.out_audio_only         = fcgCBAudioOnly->Checked;
    cnf->aud.faw_check              = fcgCBFAWCheck->Checked;
    cnf->aud.enc_mode               = fcgCXAudioEncMode->SelectedIndex;
    cnf->aud.bitrate                = (int)fcgNUAudioBitrate->Value;
    cnf->aud.use_2pass              = fcgCBAudio2pass->Checked;
    cnf->aud.use_wav                = !fcgCBAudioUsePipe->Checked;
    cnf->aud.delay_cut              = fcgCXAudioDelayCut->SelectedIndex;
    cnf->aud.priority               = fcgCXAudioPriority->SelectedIndex;
    cnf->aud.audio_encode_timing    = fcgCXAudioEncTiming->SelectedIndex;
    cnf->aud.aud_temp_dir           = fcgCXAudioTempDir->SelectedIndex;

    //mux部
    cnf->mux.disable_mp4ext         = !fcgCBMP4MuxerExt->Checked;
    cnf->mux.apple_mode             = fcgCBMP4MuxApple->Checked;
    cnf->mux.mp4_mode               = fcgCXMP4CmdEx->SelectedIndex;
    cnf->mux.mp4_temp_dir           = fcgCXMP4BoxTempDir->SelectedIndex;
    cnf->mux.disable_mkvext         = !fcgCBMKVMuxerExt->Checked;
    cnf->mux.mkv_mode               = fcgCXMKVCmdEx->SelectedIndex;
    cnf->mux.disable_mpgext         = !fcgCBMPGMuxerExt->Checked;
    cnf->mux.mpg_mode               = fcgCXMPGCmdEx->SelectedIndex;
    cnf->mux.minimized              = fcgCBMuxMinimize->Checked;
    cnf->mux.priority               = fcgCXMuxPriority->SelectedIndex;
    
    cnf->oth.run_bat                = RUN_BAT_NONE;
    cnf->oth.run_bat               |= (fcgCBRunBatBeforeAudio->Checked) ? RUN_BAT_BEFORE_AUDIO   : NULL;
    cnf->oth.run_bat               |= (fcgCBRunBatAfterAudio->Checked)  ? RUN_BAT_AFTER_AUDIO    : NULL;
    cnf->oth.run_bat               |= (fcgCBRunBatBefore->Checked)      ? RUN_BAT_BEFORE_PROCESS : NULL;
    cnf->oth.run_bat               |= (fcgCBRunBatAfter->Checked)       ? RUN_BAT_AFTER_PROCESS  : NULL;
    cnf->oth.dont_wait_bat_fin      = RUN_BAT_NONE;
    cnf->oth.dont_wait_bat_fin     |= (!fcgCBWaitForBatBefore->Checked) ? RUN_BAT_BEFORE_PROCESS : NULL;
    cnf->oth.dont_wait_bat_fin     |= (!fcgCBWaitForBatAfter->Checked)  ? RUN_BAT_AFTER_PROCESS  : NULL;
    GetCHARfromString(cnf->oth.batfile.before_process, sizeof(cnf->oth.batfile.before_process), fcgTXBatBeforePath->Text);
    GetCHARfromString(cnf->oth.batfile.after_process,  sizeof(cnf->oth.batfile.after_process),  fcgTXBatAfterPath->Text);
    GetCHARfromString(cnf->oth.batfile.before_audio, sizeof(cnf->oth.batfile.before_audio), fcgTXBatBeforeAudioPath->Text);
    GetCHARfromString(cnf->oth.batfile.after_audio,  sizeof(cnf->oth.batfile.after_audio),  fcgTXBatAfterAudioPath->Text);
}

System::Void frmConfig::GetfcgTSLSettingsNotes(char *notes, int nSize) {
    ZeroMemory(notes, nSize);
    if (fcgTSLSettingsNotes->ForeColor == Color::FromArgb(StgNotesColor[0][0], StgNotesColor[0][1], StgNotesColor[0][2]))
        GetCHARfromString(notes, nSize, fcgTSLSettingsNotes->Text);
}

System::Void frmConfig::SetfcgTSLSettingsNotes(const char *notes) {
    if (str_has_char(notes)) {
        fcgTSLSettingsNotes->ForeColor = Color::FromArgb(StgNotesColor[0][0], StgNotesColor[0][1], StgNotesColor[0][2]);
        fcgTSLSettingsNotes->Text = String(notes).ToString();
    } else {
        fcgTSLSettingsNotes->ForeColor = Color::FromArgb(StgNotesColor[1][0], StgNotesColor[1][1], StgNotesColor[1][2]);
        fcgTSLSettingsNotes->Text = String(DefaultStgNotes).ToString();
    }
}

System::Void frmConfig::SetfcgTSLSettingsNotes(String^ notes) {
    if (notes->Length && String::Compare(notes, String(DefaultStgNotes).ToString()) != 0) {
        fcgTSLSettingsNotes->ForeColor = Color::FromArgb(StgNotesColor[0][0], StgNotesColor[0][1], StgNotesColor[0][2]);
        fcgTSLSettingsNotes->Text = notes;
    } else {
        fcgTSLSettingsNotes->ForeColor = Color::FromArgb(StgNotesColor[1][0], StgNotesColor[1][1], StgNotesColor[1][2]);
        fcgTSLSettingsNotes->Text = String(DefaultStgNotes).ToString();
    }
}

System::Void frmConfig::SetChangedEvent(Control^ control, System::EventHandler^ _event) {
    System::Type^ ControlType = control->GetType();
    if (ControlType == NumericUpDown::typeid)
        ((NumericUpDown^)control)->ValueChanged += _event;
    else if (ControlType == ComboBox::typeid)
        ((ComboBox^)control)->SelectedIndexChanged += _event;
    else if (ControlType == CheckBox::typeid)
        ((CheckBox^)control)->CheckedChanged += _event;
    else if (ControlType == TextBox::typeid)
        ((TextBox^)control)->TextChanged += _event;
}

System::Void frmConfig::SetToolStripEvents(ToolStrip^ TS, System::Windows::Forms::MouseEventHandler^ _event) {
    for (int i = 0; i < TS->Items->Count; i++) {
        ToolStripButton^ TSB = dynamic_cast<ToolStripButton^>(TS->Items[i]);
        if (TSB != nullptr) TSB->MouseDown += _event;
    }
}

System::Void frmConfig::SetAllCheckChangedEvents(Control ^top) {
    //再帰を使用してすべてのコントロールのtagを調べ、イベントをセットする
    for (int i = 0; i < top->Controls->Count; i++) {
        System::Type^ type = top->Controls[i]->GetType();
        if (type == NumericUpDown::typeid)
            top->Controls[i]->Enter += gcnew System::EventHandler(this, &frmConfig::NUSelectAll);

        if (type == Label::typeid || type == Button::typeid)
            ;
        else if (type == ToolStrip::typeid)
            SetToolStripEvents((ToolStrip^)(top->Controls[i]), gcnew System::Windows::Forms::MouseEventHandler(this, &frmConfig::fcgTSItem_MouseDown));
        else if (top->Controls[i]->Tag == nullptr)
            SetAllCheckChangedEvents(top->Controls[i]);
        else if (String::Equals(top->Controls[i]->Tag->ToString(), L"chValue"))
            SetChangedEvent(top->Controls[i], gcnew System::EventHandler(this, &frmConfig::CheckOtherChanges));
        else
            SetAllCheckChangedEvents(top->Controls[i]);
    }
}

System::Void frmConfig::SetHelpToolTips() {

    //拡張
    fcgTTEx->SetToolTip(fcgCXTempDir,      L""
        + L"一時ファイル群\n"
        + L"・音声一時ファイル(wav / エンコード後音声)\n"
        + L"・動画一時ファイル\n"
        + L"・タイムコードファイル\n"
        + L"・qpファイル\n"
        + L"・mux後ファイル\n"
        + L"の作成場所を指定します。"
        );
    fcgTTEx->SetToolTip(fcgCBAFS,                L""
        + L"自動フィールドシフト(afs)を使用してVFR化を行います。\n"
        + L"エンコード時にタイムコードを作成し、mux時に埋め込んで\n"
        + L"フレームレートを変更します。\n"
        + L"\n"
        + L"あとからフレームレートを変更するため、\n"
        + L"ビットレート設定が正確に反映されなくなる点に注意してください。"
        );
    fcgTTEx->SetToolTip(fcgCBAuoTcfileout, L""
        + L"タイムコードを出力します。このタイムコードは\n"
        + L"自動フィールドシフト(afs)を反映したものになります。"
        );

    //音声
    fcgTTEx->SetToolTip(fcgCXAudioEncoder, L""
        + L"使用する音声エンコーダを指定します。\n"
        + L"これらの設定はNVEnc.iniに記述されています。"
        );
    fcgTTEx->SetToolTip(fcgCBAudioOnly,    L""
        + L"動画の出力を行わず、音声エンコードのみ行います。\n"
        + L"音声エンコードに失敗した場合などに使用してください。"
        );
    fcgTTEx->SetToolTip(fcgCBFAWCheck,     L""
        + L"音声エンコード時に音声がFakeAACWav(FAW)かどうかの判定を行い、\n"
        + L"FAWだと判定された場合、設定を無視して、\n"
        + L"自動的にFAWを使用するよう切り替えます。\n"
        + L"\n"
        + L"一度音声エンコーダからFAW(fawcl)を選択し、\n"
        + L"実行ファイルの場所を指定しておく必要があります。"
        );
    fcgTTEx->SetToolTip(fcgBTAudioEncoderPath, L""
        + L"音声エンコーダの場所を指定します。\n"
        + L"\n"
        + L"この設定はx264guiEx.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    fcgTTEx->SetToolTip(fcgCXAudioEncMode, L""
        + L"音声エンコーダのエンコードモードを切り替えます。\n"
        + L"これらの設定はNVEnc.iniに記述されています。"
        );
    fcgTTEx->SetToolTip(fcgCBAudio2pass,   L""
        + L"音声エンコードを2passで行います。\n"
        + L"2pass時はパイプ処理は行えません。"
        );
    fcgTTEx->SetToolTip(fcgCBAudioUsePipe, L""
        + L"パイプを通して、音声データをエンコーダに渡します。\n"
        + L"パイプと2passは同時に指定できません。"
        );
    fcgTTEx->SetToolTip(fcgNUAudioBitrate, L""
        + L"音声ビットレートを指定します。"
        );
    fcgTTEx->SetToolTip(fcgCXAudioPriority, L""
        + L"音声エンコーダのCPU優先度を設定します。\n"
        + L"AviutlSync で Aviutlの優先度と同じになります。"
        );
    fcgTTEx->SetToolTip(fcgCXAudioEncTiming, L""
        + L"音声を処理するタイミングを設定します。\n"
        + L" 後　 … 映像→音声の順で処理します。\n"
        + L" 前　 … 音声→映像の順で処理します。\n"
        + L" 同時 … 映像と音声を同時に処理します。"
        );
    fcgTTEx->SetToolTip(fcgCXAudioTempDir, L""
        + L"音声一時ファイル(エンコード後のファイル)\n"
        + L"の出力先を変更します。"
        );
    fcgTTEx->SetToolTip(fcgBTCustomAudioTempDir, L""
        + L"音声一時ファイルの場所を「カスタム」にした時に\n"
        + L"使用される音声一時ファイルの場所を指定します。\n"
        + L"\n"
        + L"この設定はx264guiEx.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    //音声バッチファイル実行
    fcgTTEx->SetToolTip(fcgCBRunBatBeforeAudio, L""
        + L"音声エンコード開始前にバッチファイルを実行します。"
        );
    fcgTTEx->SetToolTip(fcgCBRunBatAfterAudio, L""
        + L"音声エンコード終了後、バッチファイルを実行します。"
        );
    fcgTTEx->SetToolTip(fcgBTBatBeforeAudioPath, L""
        + L"音声エンコード終了後実行するバッチファイルを指定します。\n"
        + L"実際のバッチ実行時には新たに\"<バッチファイル名>_tmp.bat\"を作成、\n"
        + L"指定したバッチファイルの内容をコピーし、\n"
        + L"さらに特定文字列を置換して実行します。\n"
        + L"使用できる置換文字列はreadmeをご覧下さい。"
        );
    fcgTTEx->SetToolTip(fcgBTBatAfterAudioPath, L""
        + L"音声エンコード終了後実行するバッチファイルを指定します。\n"
        + L"実際のバッチ実行時には新たに\"<バッチファイル名>_tmp.bat\"を作成、\n"
        + L"指定したバッチファイルの内容をコピーし、\n"
        + L"さらに特定文字列を置換して実行します。\n"
        + L"使用できる置換文字列はreadmeをご覧下さい。"
        );

    //muxer
    fcgTTEx->SetToolTip(fcgCBMP4MuxerExt, L""
        + L"指定したmuxerでmuxを行います。\n"
        + L"チェックを外すとmuxを行いません。"
        );
    fcgTTEx->SetToolTip(fcgCXMP4CmdEx,    L""
        + L"muxerに渡す追加オプションを選択します。\n"
        + L"これらの設定はNVEnc.iniに記述されています。"
        );
    fcgTTEx->SetToolTip(fcgBTMP4MuxerPath, L""
        + L"mp4用muxerの場所を指定します。\n"
        + L"\n"
        + L"この設定はNVEnc.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    fcgTTEx->SetToolTip(fcgBTMP4RawPath, L""
        + L"raw用mp4muxerの場所を指定します。\n"
        + L"\n"
        + L"この設定はNVEnc.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    fcgTTEx->SetToolTip(fcgCXMP4BoxTempDir, L""
        + L"mp4box用の一時フォルダの場所を指定します。"
        );
    fcgTTEx->SetToolTip(fcgBTMP4BoxTempDir, L""
        + L"mp4box用一時フォルダの場所を「カスタム」に設定した際に\n"
        + L"使用される一時フォルダの場所です。\n"
        + L"\n"
        + L"この設定はNVEnc.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    fcgTTEx->SetToolTip(fcgCBMKVMuxerExt, L""
        + L"指定したmuxerでmuxを行います。\n"
        + L"チェックを外すとmuxを行いません。"
        );
    fcgTTEx->SetToolTip(fcgCXMKVCmdEx,    L""
        + L"muxerに渡す追加オプションを選択します。\n"
        + L"これらの設定はNVEnc.iniに記述されています。"
        );
    fcgTTEx->SetToolTip(fcgBTMKVMuxerPath, L""
        + L"mkv用muxerの場所を指定します。\n"
        + L"\n"
        + L"この設定はNVEnc.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    fcgTTEx->SetToolTip(fcgCBMPGMuxerExt, L""
        + L"指定したmuxerでmuxを行います。\n"
        + L"チェックを外すとmuxを行いません。"
        );
    fcgTTEx->SetToolTip(fcgCXMPGCmdEx,    L""
        + L"muxerに渡す追加オプションを選択します。\n"
        + L"これらの設定はNVEnc.iniに記述されています。"
        );
    fcgTTEx->SetToolTip(fcgBTMPGMuxerPath, L""
        + L"mpg用muxerの場所を指定します。\n"
        + L"\n"
        + L"この設定はNVEnc.confに保存され、\n"
        + L"バッチ処理ごとの変更はできません。"
        );
    fcgTTEx->SetToolTip(fcgCBMuxMinimize, L""
        + L"mux時のウィンドウを最小化で開始します。"
        );
    fcgTTEx->SetToolTip(fcgCXMuxPriority, L""
        + L"muxerのCPU優先度を指定します。\n"
        + L"AviutlSync で Aviutlの優先度と同じになります。"
        );
    //バッチファイル実行
    fcgTTEx->SetToolTip(fcgCBRunBatBefore, L""
        + L"エンコード開始前にバッチファイルを実行します。"
        );
    fcgTTEx->SetToolTip(fcgCBRunBatAfter, L""
        + L"エンコード終了後、バッチファイルを実行します。"
        );
    fcgTTEx->SetToolTip(fcgCBWaitForBatBefore, L""
        + L"バッチ処理開始後、バッチ処理が終了するまで待機します。"
        );
    fcgTTEx->SetToolTip(fcgCBWaitForBatAfter, L""
        + L"バッチ処理開始後、バッチ処理が終了するまで待機します。"
        );
    fcgTTEx->SetToolTip(fcgBTBatBeforePath, L""
        + L"エンコード終了後実行するバッチファイルを指定します。\n"
        + L"実際のバッチ実行時には新たに\"<バッチファイル名>_tmp.bat\"を作成、\n"
        + L"指定したバッチファイルの内容をコピーし、\n"
        + L"さらに特定文字列を置換して実行します。\n"
        + L"使用できる置換文字列はreadmeをご覧下さい。"
        );
    fcgTTEx->SetToolTip(fcgBTBatAfterPath, L""
        + L"エンコード終了後実行するバッチファイルを指定します。\n"
        + L"実際のバッチ実行時には新たに\"<バッチファイル名>_tmp.bat\"を作成、\n"
        + L"指定したバッチファイルの内容をコピーし、\n"
        + L"さらに特定文字列を置換して実行します。\n"
        + L"使用できる置換文字列はreadmeをご覧下さい。"
        );
    //上部ツールストリップ
    fcgTSBDelete->ToolTipText = L""
        + L"現在選択中のプロファイルを削除します。";

    fcgTSBOtherSettings->ToolTipText = L""
        + L"プロファイルの保存フォルダを変更します。";

    fcgTSBSave->ToolTipText = L""
        + L"現在の設定をプロファイルに上書き保存します。";

    fcgTSBSaveNew->ToolTipText = L""
        + L"現在の設定を新たなプロファイルに保存します。";

    //他
    fcgTTEx->SetToolTip(fcgTXCmd,         L""
        + L"x264に渡される予定のコマンドラインです。\n"
        + L"エンコード時には更に\n"
        + L"・「追加コマンド」の付加\n"
        + L"・\"auto\"な設定項目の反映\n"
        + L"・必要な情報の付加(--fps/-o/--input-res/--input-csp/--frames等)\n"
        + L"が行われます。\n"
        + L"\n"
        + L"このウィンドウはダブルクリックで拡大縮小できます。"
        );
    fcgTTEx->SetToolTip(fcgBTDefault,     L""
        + L"デフォルト設定をロードします。"
        );
}
System::Void frmConfig::ShowExehelp(String^ ExePath, String^ args) {
    if (!File::Exists(ExePath)) {
        MessageBox::Show(L"指定された実行ファイルが存在しません。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
    } else {
        char exe_path[MAX_PATH_LEN];
        char file_path[MAX_PATH_LEN];
        char cmd[MAX_CMD_LEN];
        GetCHARfromString(exe_path, sizeof(exe_path), ExePath);
        apply_appendix(file_path, _countof(file_path), exe_path, "_fullhelp.txt");
        File::Delete(String(file_path).ToString());
        array<String^>^ arg_list = args->Split(L';');
        for (int i = 0; i < arg_list->Length; i++) {
            if (i) {
                StreamWriter^ sw;
                try {
                    sw = gcnew StreamWriter(String(file_path).ToString(), true, System::Text::Encoding::GetEncoding("shift_jis"));
                    sw->WriteLine();
                    sw->WriteLine();
                } catch (...) {
                    //ファイルオープンに失敗…初回のget_exe_message_to_fileでエラーとなるため、おそらく起こらない
                } finally {
                    if (sw != nullptr) { sw->Close(); }
                }
            }
            GetCHARfromString(cmd, sizeof(cmd), arg_list[i]);
            if (get_exe_message_to_file(exe_path, cmd, file_path, AUO_PIPE_MUXED, 5) != RP_SUCCESS) {
                File::Delete(String(file_path).ToString());
                MessageBox::Show(L"helpの取得に失敗しました。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }
        }
        try {
            System::Diagnostics::Process::Start(String(file_path).ToString());
        } catch (...) {
            MessageBox::Show(L"helpを開く際に不明なエラーが発生しました。", L"エラー", MessageBoxButtons::OK, MessageBoxIcon::Error);
        }
    }
}

System::Void frmConfig::SetupFeatureTable() {
    //表示更新
    fcgDGVFeaturesH264->ReadOnly = true;
    fcgDGVFeaturesH264->AllowUserToAddRows = false;
    fcgDGVFeaturesH264->AllowUserToResizeRows = false;
    fcgDGVFeaturesH264->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;

    fcgDGVFeaturesHEVC->ReadOnly = true;
    fcgDGVFeaturesHEVC->AllowUserToAddRows = false;
    fcgDGVFeaturesHEVC->AllowUserToResizeRows = false;
    fcgDGVFeaturesHEVC->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;
    
    dataTableNVEncFeaturesH264 = gcnew DataTable();
    dataTableNVEncFeaturesH264->Columns->Add(L"機能");
    dataTableNVEncFeaturesH264->Columns->Add(L"サポート");

    dataTableNVEncFeaturesHEVC = gcnew DataTable();
    dataTableNVEncFeaturesHEVC->Columns->Add(L"機能");
    dataTableNVEncFeaturesHEVC->Columns->Add(L"サポート");

    fcgDGVFeaturesH264->DataSource = dataTableNVEncFeaturesH264; //テーブルをバインド
    fcgDGVFeaturesHEVC->DataSource = dataTableNVEncFeaturesHEVC; //テーブルをバインド
}

System::Void frmConfig::SetEnvironmentInfo() {
    //CPU名
    if (nullptr == StrCPUInfo || StrCPUInfo->Length <= 0) {
        TCHAR cpu_info[256] = { 0 };
        getCPUInfo(cpu_info, _countof(cpu_info));
        StrCPUInfo = String(cpu_info).ToString()->Replace(L" CPU ", L" ");
    }
    //GPU名
    if (nullptr == StrGPUInfo || StrGPUInfo->Length <= 0) {
        TCHAR gpu_info[256] = { 0 };
        getGPUInfo(GPU_VENDOR, gpu_info, _countof(gpu_info));
        StrGPUInfo = String(gpu_info).ToString();
    }

    //機能情報
    if (featureCache && dataTableNVEncFeaturesH264->Rows->Count <= 1) {
        nvfeature_GetCachedNVEncCapability(featureCache);
    }
    if (this->InvokeRequired) {
        SetEnvironmentInfoDelegate^ sl = gcnew SetEnvironmentInfoDelegate(this, &frmConfig::SetEnvironmentInfo);
        this->Invoke(sl);
    } else {
        OSVERSIONINFOEXW osversioninfo = { 0 };
        tstring osversionstr = getOSVersion(&osversioninfo);
        fcgLBOSInfo->Text = String(getOSVersion().c_str()).ToString() + (is_64bit_os() ? String(L" x64 ").ToString() : String(L" x86 ").ToString() + osversioninfo.dwBuildNumber.ToString());
        fcgLBCPUInfoOnFeatureTab->Text = StrCPUInfo;
        fcgLBGPUInfoOnFeatureTab->Text = StrGPUInfo;
        if (featureCache) {
            auto nvencCapabilities = nvfeature_GetCachedNVEncCapability(featureCache);
            auto hevcFeatures = nvfeature_GetHEVCFeatures(nvencCapabilities);
            auto h264Features = nvfeature_GetH264Features(nvencCapabilities);
            if (nullptr != h264Features) {
                for (auto cap : h264Features->caps) {
                    DataRow^ drb = dataTableNVEncFeaturesH264->NewRow();
                    drb[0] = String(cap.name).ToString();
                    drb[1] = cap.value.ToString();
                    dataTableNVEncFeaturesH264->Rows->Add(drb);
                }
            }
            if (nullptr != hevcFeatures) {
                for (auto cap : hevcFeatures->caps) {
                    DataRow^ drb = dataTableNVEncFeaturesHEVC->NewRow();
                    drb[0] = String(cap.name).ToString();
                    drb[1] = cap.value.ToString();
                    dataTableNVEncFeaturesHEVC->Rows->Add(drb);
                }
            }
        }
    }
}

#pragma warning( pop )
