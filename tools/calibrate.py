#!/usr/bin/env python3
"""Calibrazione del clock del R080 Timer (TECHNICAL C-29, REQ-9.13).

1. Sul timer: Menu di servizio > Calibrazione > Log calibrazione, clic.
2. Sul PC:    python3 tools/calibrate.py /dev/ttyACM0      (Windows: COM3)
3. Dopo 30 minuti lo script stampa la Correzione del clock in ppm:
   inserirla in Menu di servizio > Calibrazione > Correzione ppm (`*` cambia il segno).

Il firmware scrive una riga `CAL ms=<conteggio>` al secondo con i tick di Timer1 senza
correzione; lo script registra per ognuna l'istante del PC e stima la pendenza con una
regressione lineare, che assorbe il ritardo variabile della USB.

Verifica senza scheda: python3 tools/calibrate.py --self-test
Dipendenze: libreria standard (Python 3.10+) e pyserial.
"""
import argparse
import random
import re
import statistics
import sys
import time

CAL = re.compile(rb"CAL ms=(\d+)")


def estimate_ppm(samples):
    """Correzione del clock da coppie (conteggio ms della scheda, istante del PC in ns).

    Positiva se la scheda è lenta: ogni suo ms dura più di un ms del PC e va contato di più.
    """
    x0, y0 = samples[0]
    xs = [x - x0 for x, _ in samples]
    ys = [(y - y0) / 1e6 for _, y in samples]  # ms del PC dal primo campione
    slope, _ = statistics.linear_regression(xs, ys)
    return (slope - 1) * 1e6


def read_samples(port, minutes):
    import serial  # pyserial

    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 2
    s.dtr = s.rts = False  # aprire la porta non deve riavviare il Mega e chiudere il menu
    s.open()
    samples = []
    end = time.monotonic() + minutes * 60
    with s:
        while time.monotonic() < end:
            line = s.readline()
            now = time.monotonic_ns()
            m = CAL.search(line)
            if m:
                samples.append((int(m.group(1)), now))
                left = int(end - time.monotonic())
                print(f"\r{len(samples)} righe, {left // 60:02d}:{left % 60:02d} alla fine ", end="",
                      file=sys.stderr, flush=True)
    print(file=sys.stderr)
    return samples


def self_test():
    """Log sintetico a deriva nota, con fino a 4 ms di ritardo USB casuale per riga."""
    rng = random.Random(14)
    for ppm in (-1000, -200, -37, 0, 37, 200, 1000):
        samples = [(12345 + 1000 * k, round(1000 * k * (1 + ppm / 1e6) * 1e6 + rng.uniform(0, 4e6)))
                   for k in range(1800)]
        got = estimate_ppm(samples)
        assert abs(got - ppm) < 1, f"deriva {ppm} ppm, stimata {got:.2f}"
    print("self-test OK")


def main():
    p = argparse.ArgumentParser(description="Stima la Correzione del clock del R080 Timer dal log di calibrazione.")
    p.add_argument("port", nargs="?", help="porta USB del timer, per esempio /dev/ttyACM0 o COM3")
    p.add_argument("--minutes", type=float, default=30, help="durata della misura (predefinita 30)")
    p.add_argument("--self-test", action="store_true", help="verifica la stima su un log sintetico")
    a = p.parse_args()
    if a.self_test:
        return self_test()
    if not a.port:
        p.error("manca la porta USB")
    samples = read_samples(a.port, a.minutes)
    if len(samples) < 2:
        sys.exit("Nessuna riga CAL: avviare il log di calibrazione dal Menu di servizio.")
    ppm = estimate_ppm(samples)
    print(f"Correzione del clock: {round(ppm):+d} ppm ({len(samples)} righe)")
    if abs(ppm) > 1000:
        print("Fuori dall'intervallo ±1000 ppm: controllare la misura.")


if __name__ == "__main__":
    main()
