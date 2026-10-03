import sys
import json
import os
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QGridLayout, QLabel, QPushButton,
                             QSpinBox, QComboBox, QFileDialog, QMessageBox,
                             QSlider, QListWidget, QAbstractItemView, QButtonGroup)
from PyQt6.QtCore import Qt, pyqtSignal

# ==========================================
# 1. MODEL
# ==========================================


class SequencerModel:
    def __init__(self):
        self.reset_to_default()

    def reset_to_default(self):
        self.bpm = 120
        self.playMode = 0
        self.playlist = [0]
        self.patterns = [{"s": [0]*8, "sw": [0]*8} for _ in range(64)]

    def load_from_file(self, filepath):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)

            self.bpm = max(30, min(300, data.get("bpm", 120)))
            self.playMode = data.get("playMode", 0)
            if self.playMode not in [0, 1]:
                self.playMode = 0

            self.playlist = data.get("playlist", [])[:128]

            loaded_patterns = data.get("patterns", [])
            self.patterns = [{"s": [0]*8, "sw": [0]*8} for _ in range(64)]
            for i, p in enumerate(loaded_patterns):
                if i < 64:
                    self.patterns[i]["s"] = p.get("s", [0]*8)[:8]
                    self.patterns[i]["sw"] = p.get("sw", [0]*8)[:8]

            return True
        except Exception as e:
            print(f"Error loading file: {e}")
            return False

    def save_to_file(self, filepath):
        try:
            data = {
                "bpm": self.bpm,
                "playMode": self.playMode,
                "playlistLength": len(self.playlist),
                "playlist": self.playlist,
                "patterns": self.patterns
            }
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving file: {e}")
            return False

    def get_step(self, pattern_idx, track_idx, step_idx):
        return (self.patterns[pattern_idx]["s"][track_idx] >> step_idx) & 1

    def toggle_step(self, pattern_idx, track_idx, step_idx):
        self.patterns[pattern_idx]["s"][track_idx] ^= (1 << step_idx)
        return self.get_step(pattern_idx, track_idx, step_idx)

    def set_swing(self, pattern_idx, track_idx, value):
        self.patterns[pattern_idx]["sw"][track_idx] = max(0, min(100, value))

    def get_swing(self, pattern_idx, track_idx):
        return self.patterns[pattern_idx]["sw"][track_idx]

# ==========================================
# 2. CUSTOM WIDGETS (VIEWS)
# ==========================================


class StepButton(QPushButton):
    """Custom button acting as a sequencer pad"""

    def __init__(self, track_idx, step_idx):
        super().__init__()
        self.track_idx = track_idx
        self.step_idx = step_idx
        self.is_active = False

        group = (step_idx // 4) % 2
        self.base_color = "#3a3a3a" if group == 0 else "#2d2d2d"
        self.active_color = "#ff9900"  # Bright amber

        self.setFixedSize(30, 30)
        self.update_style()
        self.setCursor(Qt.CursorShape.PointingHandCursor)

    def set_active(self, active):
        self.is_active = active
        self.update_style()

    def update_style(self):
        color = self.active_color if self.is_active else self.base_color
        border = "1px solid #ffcc00" if self.is_active else "1px solid #1a1a1a"
        self.setStyleSheet(f"""
            QPushButton {{
                background-color: {color};
                border: {border};
                border-radius: 4px;
            }}
            QPushButton:hover {{
                border: 1px solid #ffffff;
            }}
        """)


class PatternButton(QPushButton):
    """Custom button for the 16-pad pattern selector"""

    def __init__(self, index):
        super().__init__()
        self.index = index
        self.is_active = False
        self.base_color = "#2d2d2d"
        self.active_color = "#007acc"  # Bright blue

        self.setFixedSize(30, 30)
        self.update_style()
        self.setCursor(Qt.CursorShape.PointingHandCursor)

    def set_active(self, active):
        self.is_active = active
        self.update_style()

    def update_style(self):
        color = self.active_color if self.is_active else self.base_color
        border = "1px solid #00aaff" if self.is_active else "1px solid #1a1a1a"
        font_weight = "bold" if self.is_active else "normal"
        self.setStyleSheet(f"""
            QPushButton {{
                background-color: {color};
                border: {border};
                border-radius: 4px;
                color: white;
                font-size: 10px;
                font-weight: {font_weight};
            }}
            QPushButton:hover {{
                border: 1px solid #ffffff;
            }}
        """)

# ==========================================
# 3. MAIN CONTROLLER / WINDOW
# ==========================================


class SequencerEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        self.model = SequencerModel()
        self.current_pattern_idx = 0
        self.current_bank_idx = 0

        self.setWindowTitle("8-Track Sequencer Editor")
        self.setMinimumSize(1000, 650)

        self.init_ui()
        self.sync_ui_to_model()

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # --- A. Header / Global Transport Bar ---
        header_layout = QHBoxLayout()

        btn_open = QPushButton("Open")
        btn_save = QPushButton("Save")
        btn_save_as = QPushButton("Save As...")
        btn_open.clicked.connect(self.action_open)
        btn_save.clicked.connect(lambda: self.action_save())
        btn_save_as.clicked.connect(lambda: self.action_save(save_as=True))

        header_layout.addWidget(btn_open)
        header_layout.addWidget(btn_save)
        header_layout.addWidget(btn_save_as)
        header_layout.addSpacing(20)

        header_layout.addWidget(QLabel("BPM:"))
        self.spin_bpm = QSpinBox()
        self.spin_bpm.setRange(30, 300)
        self.spin_bpm.valueChanged.connect(self.on_bpm_changed)
        header_layout.addWidget(self.spin_bpm)

        header_layout.addSpacing(20)

        header_layout.addWidget(QLabel("Play Mode:"))
        self.combo_mode = QComboBox()
        self.combo_mode.addItems(["Pattern Loop", "Song Mode"])
        self.combo_mode.currentIndexChanged.connect(self.on_mode_changed)
        header_layout.addWidget(self.combo_mode)
        header_layout.addStretch()

        main_layout.addLayout(header_layout)
        main_layout.addSpacing(10)

        # --- Middle Split (Grid + Playlist) ---
        body_layout = QHBoxLayout()

        # --- B. Pattern Editor Grid ---
        grid_container = QWidget()
        grid_vbox = QVBoxLayout(grid_container)

        # Bank Selectors
        bank_layout = QHBoxLayout()
        bank_layout.addWidget(QLabel("Bank:"))

        self.bank_group = QButtonGroup(self)
        self.bank_btns = []
        bank_labels = ["1-16", "17-32", "33-48", "49-64"]
        for i in range(4):
            btn = QPushButton(bank_labels[i])
            btn.setCheckable(True)
            self.bank_group.addButton(btn, i)
            btn.clicked.connect(lambda checked, b=i: self.on_bank_clicked(b))
            self.bank_btns.append(btn)
            bank_layout.addWidget(btn)
        bank_layout.addStretch()
        grid_vbox.addLayout(bank_layout)

        self.grid_layout = QGridLayout()
        self.grid_layout.setSpacing(5)

        # Row 0: Pattern Selector Buttons
        lbl_pat = QLabel("Pattern")
        lbl_pat.setAlignment(Qt.AlignmentFlag.AlignRight |
                             Qt.AlignmentFlag.AlignVCenter)
        self.grid_layout.addWidget(lbl_pat, 0, 0)

        self.pattern_btns = []
        for col in range(16):
            btn = PatternButton(col)
            btn.clicked.connect(
                lambda checked, p=col: self.on_pattern_btn_clicked(p))
            self.grid_layout.addWidget(btn, 0, col + 1)
            self.pattern_btns.append(btn)

        # Add visual separator
        sep = QLabel("")
        sep.setFixedHeight(10)
        self.grid_layout.addWidget(sep, 1, 0, 1, 18)

        # Row 2: Step Column Labels (1-16)
        for col in range(16):
            lbl = QLabel(str(col + 1))
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl.setStyleSheet("color: #888888; font-size: 10px;")
            self.grid_layout.addWidget(lbl, 2, col + 1)

        self.grid_layout.addWidget(QLabel("Swing %"), 2, 17)

        self.step_buttons = []
        self.swing_spins = []
        track_names = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H']

        # Rows 3-10: Tracks
        for t_idx in range(8):
            row = t_idx + 3

            lbl = QLabel(track_names[t_idx])
            lbl.setAlignment(Qt.AlignmentFlag.AlignRight |
                             Qt.AlignmentFlag.AlignVCenter)
            self.grid_layout.addWidget(lbl, row, 0)

            track_btns = []
            for s_idx in range(16):
                btn = StepButton(t_idx, s_idx)
                btn.clicked.connect(lambda checked, t=t_idx,
                                    s=s_idx: self.on_step_clicked(t, s))
                self.grid_layout.addWidget(btn, row, s_idx + 1)
                track_btns.append(btn)
            self.step_buttons.append(track_btns)

            swing_layout = QHBoxLayout()
            swing_slider = QSlider(Qt.Orientation.Horizontal)
            swing_slider.setRange(0, 100)
            swing_slider.setFixedWidth(80)

            swing_val = QLabel("0")
            swing_val.setFixedWidth(20)

            swing_slider.valueChanged.connect(
                lambda val, t=t_idx, l=swing_val: self.on_swing_changed(t, val, l))

            swing_layout.addWidget(swing_slider)
            swing_layout.addWidget(swing_val)
            self.grid_layout.addLayout(swing_layout, row, 17)
            self.swing_spins.append((swing_slider, swing_val))

        grid_vbox.addLayout(self.grid_layout)

        grid_vbox.addStretch()

        body_layout.addWidget(grid_container, stretch=3)

        # --- C. Playlist / Song Builder ---
        playlist_container = QWidget()
        playlist_layout = QVBoxLayout(playlist_container)
        playlist_layout.addWidget(QLabel("Playlist (Double-click to edit)"))

        self.list_widget = QListWidget()
        self.list_widget.setSelectionMode(
            QAbstractItemView.SelectionMode.SingleSelection)
        self.list_widget.itemDoubleClicked.connect(
            self.on_playlist_double_clicked)
        playlist_layout.addWidget(self.list_widget)

        plist_btns_layout = QHBoxLayout()
        btn_add = QPushButton("+ Add")
        btn_remove = QPushButton("- Remove")
        btn_up = QPushButton("Up")
        btn_down = QPushButton("Down")

        btn_add.clicked.connect(self.playlist_add)
        btn_remove.clicked.connect(self.playlist_remove)
        btn_up.clicked.connect(self.playlist_move_up)
        btn_down.clicked.connect(self.playlist_move_down)

        plist_btns_layout.addWidget(btn_add)
        plist_btns_layout.addWidget(btn_remove)
        plist_btns_layout.addWidget(btn_up)
        plist_btns_layout.addWidget(btn_down)

        playlist_layout.addLayout(plist_btns_layout)
        body_layout.addWidget(playlist_container, stretch=1)

        main_layout.addLayout(body_layout)
        self.current_file = None

    # --- UI Sync & Controllers ---
    def sync_ui_to_model(self):
        self.spin_bpm.blockSignals(True)
        self.spin_bpm.setValue(self.model.bpm)
        self.spin_bpm.blockSignals(False)

        self.combo_mode.blockSignals(True)
        self.combo_mode.setCurrentIndex(self.model.playMode)
        self.combo_mode.blockSignals(False)

        # Ensure bank matches the loaded pattern
        self.current_bank_idx = self.current_pattern_idx // 16

        self.refresh_pattern_bank_ui()
        self.refresh_grid()
        self.refresh_playlist()

    def refresh_pattern_bank_ui(self):
        """Updates the bank radio buttons and the 16 pattern labels/highlights"""
        self.bank_btns[self.current_bank_idx].setChecked(True)

        for i in range(16):
            pattern_number = (self.current_bank_idx * 16) + i
            self.pattern_btns[i].setText(str(pattern_number + 1))

            # Highlight if it's the currently active pattern
            is_active = (pattern_number == self.current_pattern_idx)
            self.pattern_btns[i].set_active(is_active)

    def refresh_grid(self):
        p_idx = self.current_pattern_idx
        for t_idx in range(8):
            for s_idx in range(16):
                is_on = self.model.get_step(p_idx, t_idx, s_idx)
                self.step_buttons[t_idx][s_idx].set_active(bool(is_on))

            swing_val = self.model.get_swing(p_idx, t_idx)
            slider, label = self.swing_spins[t_idx]
            slider.blockSignals(True)
            slider.setValue(swing_val)
            label.setText(str(swing_val))
            slider.blockSignals(False)

    def refresh_playlist(self):
        self.list_widget.clear()
        for idx, pattern_id in enumerate(self.model.playlist):
            self.list_widget.addItem(f"{idx + 1}: Pattern {pattern_id + 1}")

    # --- Event Handlers ---
    def on_bank_clicked(self, bank_idx):
        self.current_bank_idx = bank_idx
        # Simply viewing a bank doesn't change the active playing pattern,
        # it just re-labels the 16 buttons so you can select one.
        self.refresh_pattern_bank_ui()

    def on_pattern_btn_clicked(self, sub_idx):
        # Calculate new absolute pattern ID (0-63)
        self.current_pattern_idx = (self.current_bank_idx * 16) + sub_idx
        self.refresh_pattern_bank_ui()
        self.refresh_grid()

    def on_step_clicked(self, t_idx, s_idx):
        new_state = self.model.toggle_step(
            self.current_pattern_idx, t_idx, s_idx)
        self.step_buttons[t_idx][s_idx].set_active(bool(new_state))

    def on_swing_changed(self, t_idx, value, label_widget):
        self.model.set_swing(self.current_pattern_idx, t_idx, value)
        label_widget.setText(str(value))

    def on_bpm_changed(self, val):
        self.model.bpm = val

    def on_mode_changed(self, idx):
        self.model.playMode = idx

    def on_playlist_double_clicked(self, item):
        row = self.list_widget.row(item)
        pattern_id = self.model.playlist[row]

        self.current_pattern_idx = pattern_id
        self.current_bank_idx = pattern_id // 16

        self.refresh_pattern_bank_ui()
        self.refresh_grid()

    # --- Playlist Logic ---
    def playlist_add(self):
        if len(self.model.playlist) >= 128:
            QMessageBox.warning(self, "Limit Reached",
                                "Playlist cannot exceed 128 items.")
            return
        self.model.playlist.append(self.current_pattern_idx)
        self.refresh_playlist()
        self.list_widget.setCurrentRow(len(self.model.playlist)-1)

    def playlist_remove(self):
        row = self.list_widget.currentRow()
        if row >= 0:
            self.model.playlist.pop(row)
            self.refresh_playlist()

    def playlist_move_up(self):
        row = self.list_widget.currentRow()
        if row > 0:
            item = self.model.playlist.pop(row)
            self.model.playlist.insert(row - 1, item)
            self.refresh_playlist()
            self.list_widget.setCurrentRow(row - 1)

    def playlist_move_down(self):
        row = self.list_widget.currentRow()
        if row >= 0 and row < len(self.model.playlist) - 1:
            item = self.model.playlist.pop(row)
            self.model.playlist.insert(row + 1, item)
            self.refresh_playlist()
            self.list_widget.setCurrentRow(row + 1)

    # --- File I/O Logic ---
    def action_open(self):
        filepath, _ = QFileDialog.getOpenFileName(
            self, "Open Sequencer State", "", "JSON Files (*.json)")
        if filepath:
            if self.model.load_from_file(filepath):
                self.current_file = filepath
                self.sync_ui_to_model()
            else:
                QMessageBox.critical(
                    self, "Error", "Failed to load state file.")

    def action_save(self, save_as=False):
        if not self.current_file or save_as:
            filepath, _ = QFileDialog.getSaveFileName(
                self, "Save Sequencer State", "state.json", "JSON Files (*.json)")
            if not filepath:
                return
            self.current_file = filepath

        if self.model.save_to_file(self.current_file):
            QMessageBox.information(
                self, "Success", "File saved successfully.")
        else:
            QMessageBox.critical(self, "Error", "Failed to save state file.")


if __name__ == "__main__":
    app = QApplication(sys.argv)

    app.setStyle("Fusion")
    app.setStyleSheet("""
        QMainWindow, QWidget { background-color: #1e1e1e; color: #dddddd; }
        QLabel { color: #dddddd; }
        QPushButton { background-color: #333333; border: 1px solid #555555; padding: 5px; border-radius: 3px; }
        QPushButton:hover { background-color: #444444; }
        QPushButton:checked { background-color: #007acc; color: white; border: 1px solid #00aaff; }
        QSpinBox, QComboBox, QListWidget { background-color: #2d2d2d; color: #dddddd; border: 1px solid #555555; }
    """)

    window = SequencerEditor()
    window.show()
    sys.exit(app.exec())
