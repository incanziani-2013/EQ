# AQEQ v1

Ecualizador de 6 bandas con estetica Frutiger Aero, hecho con JUCE 8 (VST3 para Windows).

## Bandas
- LOW CUT: filtro pasa-altos (FREQ). En 20 Hz queda apagado (OFF).
- LOW SHELF: FREQ, GAIN (+-15 dB).
- LOW MID: campana con FREQ, GAIN (+-15 dB) y Q.
- HIGH MID: campana con FREQ, GAIN (+-15 dB) y Q.
- HIGH SHELF: FREQ, GAIN (+-15 dB).
- HIGH CUT: filtro pasa-bajos (FREQ). En 20 kHz queda apagado (OFF).
- OUTPUT: volumen de salida (+-12 dB).

Arriba hay una curva que muestra la respuesta total del EQ en tiempo real.
Doble clic en una perilla la devuelve a su valor inicial.

## Compilar
- En la nube: sube el proyecto a GitHub; el workflow "Build AQEQ VST3" (pestana Actions) deja el VST3 como artifact "AQEQ-VST3".
- En tu PC: ejecuta build_AQEQ_vst3.bat (necesita Visual Studio con C++, CMake y JUCE 8).
