"""Regression tests for MP4 export resource handling (no GPU needed)."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import export_mp4


class ExportMP4AudioTests(unittest.TestCase):
    def test_resource_directory_uses_bundled_mp3(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executable = root / "project"
            executable.touch()
            audio = root / "build" / "music"
            audio.mkdir(parents=True)
            (audio / "cinematic.mp3").touch()
            (audio / "impact.wav").touch()
            with patch.object(export_mp4, "ROOT", root / "unrelated"):
                self.assertEqual(export_mp4.resource_directory(executable), root)

    def test_resource_directory_rejects_missing_impact(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executable = root / "project"
            executable.touch()
            audio = root / "build" / "music"
            audio.mkdir(parents=True)
            (audio / "cinematic.mp3").touch()
            with patch.object(export_mp4, "ROOT", root / "unrelated"):
                with self.assertRaisesRegex(RuntimeError, "impact.wav"):
                    export_mp4.resource_directory(executable)

    def test_decode_music_uses_ffmpeg_and_reads_duration(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source, target = root / "cinematic.mp3", root / "cinematic.wav"
            source.touch()

            def write_wav(command, check):
                self.assertTrue(check)
                self.assertEqual(command[0], "ffmpeg")
                self.assertIn(str(source), command)
                import wave
                with wave.open(str(target), "wb") as audio:
                    audio.setnchannels(2)
                    audio.setsampwidth(2)
                    audio.setframerate(48000)
                    audio.writeframes(b"\x00" * 48000 * 4)

            with patch.object(subprocess, "run", side_effect=write_wav) as runner:
                self.assertEqual(export_mp4.decode_music(source, target), 1.0)
                runner.assert_called_once()


if __name__ == "__main__":
    unittest.main()
