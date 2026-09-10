#!/usr/bin/env python3
"""
Plateforme de test du prix en 0.

Pour chaque cas `data/<nom>.json` (hors fichiers `*_expected_*`), lance
`./build/price0 data/<nom>.json`, lit le JSON produit sur stdout, et le compare
au fichier de référence `data/<nom>_expected_price.json`.

Critères (les mêmes que manquants/testForPCPD.py) :

    prix σ   = |prix_attendu - prix_obtenu|   / priceStdDev_attendu
    delta σ  = max_i |delta_attendu_i - delta_obtenu_i| / deltaStdDev_attendu_i

C'est le nombre d'écarts-types de l'estimateur Monte-Carlo qui séparent nos
résultats des références. En pratique on attend < ~2. La colonne "delta σ" vaut
"—" si price0 ne renvoie pas encore de deltas non nuls.

Usage :
    python3 scripts/compare_prices.py [--exec build/price0] [--datadir data]
                                      [--json rapport.json]
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path


def find_cases(datadir: Path) -> list[Path]:
    return sorted(
        f for f in datadir.glob("*.json")
        if "_expected_" not in f.name
    )


def run_price0(exec_path: Path, case: Path, timeout: float) -> tuple[dict | None, float, str]:
    """Retourne (resultat_json | None, duree_s, message_erreur)."""
    start = time.perf_counter()
    try:
        proc = subprocess.run(
            [str(exec_path), str(case)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=True,
        )
    except subprocess.TimeoutExpired:
        return None, timeout, f"timeout (> {timeout:.0f}s)"
    except subprocess.CalledProcessError as e:
        msg = e.stderr.decode(errors="replace").strip() or f"code de retour {e.returncode}"
        return None, time.perf_counter() - start, msg
    elapsed = time.perf_counter() - start
    try:
        return json.loads(proc.stdout.decode()), elapsed, ""
    except json.JSONDecodeError:
        return None, elapsed, "sortie non-JSON"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exec", default="build/price0", help="chemin de l'exécutable price0")
    parser.add_argument("--datadir", default="data", help="répertoire des fichiers de test")
    parser.add_argument("--timeout", type=float, default=60.0, help="timeout par cas (s)")
    parser.add_argument("--json", default=None, help="écrire le rapport détaillé dans ce fichier")
    args = parser.parse_args()

    exec_path = Path(args.exec).resolve()
    datadir = Path(args.datadir).resolve()

    if not exec_path.exists():
        print(f"Exécutable introuvable : {exec_path}\n"
              f"Compile-le d'abord :  cmake --build build --target price0", file=sys.stderr)
        return 2

    cases = find_cases(datadir)
    if not cases:
        print(f"Aucun cas de test dans {datadir}", file=sys.stderr)
        return 2

    header = (f"{'cas':<16}{'prix obt.':>12}{'prix att.':>12}{'prix σ':>9}"
              f"{'delta σ':>9}{'σ ratio':>9}{'temps':>9}   verdict")
    print(header)
    print("-" * len(header))

    report = []
    n_ok = n_warn = n_fail = n_skip = 0

    for case in cases:
        expected_path = case.with_name(case.stem + "_expected_price.json")
        result, elapsed, err = run_price0(exec_path, case, args.timeout)

        row = {"case": case.stem}

        if result is None:
            # Cas non pris en charge (ex. "performance" non implémenté) ou erreur.
            print(f"{case.stem:<16}{'—':>12}{'—':>12}{'—':>9}{'—':>9}{'—':>9}"
                  f"{elapsed:>8.2f}s   IGNORÉ ({err})")
            row.update(status="skip", error=err)
            report.append(row)
            n_skip += 1
            continue

        if not expected_path.exists():
            print(f"{case.stem:<16}{result['price']:>12.4f}{'?':>12}{'?':>9}{'?':>9}{'?':>9}"
                  f"{elapsed:>8.2f}s   PAS DE RÉFÉRENCE")
            row.update(status="no-ref", price=result["price"])
            report.append(row)
            n_skip += 1
            continue

        expected = json.loads(expected_path.read_text())
        exp_price = expected["price"]
        exp_std = expected["priceStdDev"]
        got_price = result["price"]
        got_std = result["priceStdDev"]

        price_distance = abs(exp_price - got_price) / exp_std
        std_ratio = got_std / exp_std

        # Distance des deltas : max sur les composantes de
        #   |delta_attendu[i] - delta_obtenu[i]| / deltaStdDev_attendu[i]
        # (n'utilise que l'écart-type de référence, pas le nôtre).
        exp_delta = expected.get("delta") or []
        got_delta = result.get("delta") or []
        exp_delta_std = expected.get("deltaStdDev") or []
        delta_distance = None
        if exp_delta and len(got_delta) == len(exp_delta) and any(got_delta):
            delta_distance = max(
                abs(e - g) / s if s else 0.0
                for e, g, s in zip(exp_delta, got_delta, exp_delta_std)
            )

        # Verdict : basé sur le prix ET, si disponible, sur les deltas.
        worst = price_distance if delta_distance is None else max(price_distance, delta_distance)
        if worst < 2.0:
            verdict, tag = "OK", "ok"
            n_ok += 1
        elif worst < 3.0:
            verdict, tag = "LIMITE", "warn"
            n_warn += 1
        else:
            verdict, tag = "ÉCART", "fail"
            n_fail += 1

        delta_col = "—" if delta_distance is None else f"{delta_distance:.2f}"
        print(f"{case.stem:<16}{got_price:>12.4f}{exp_price:>12.4f}"
              f"{price_distance:>9.2f}{delta_col:>9}{std_ratio:>9.2f}"
              f"{elapsed:>8.2f}s   {verdict}")

        row.update(
            status=tag,
            price_obtained=got_price, price_expected=exp_price,
            priceStdDev_obtained=got_std, priceStdDev_expected=exp_std,
            price_distance_sigma=price_distance,
            delta_distance_sigma=delta_distance,
            std_ratio=std_ratio,
            seconds=elapsed,
        )
        report.append(row)

    print("-" * len(header))
    print(f"{n_ok} OK    {n_warn} limite    {n_fail} écart    {n_skip} ignoré(s)")

    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=2))
        print(f"Rapport détaillé : {args.json}")

    # Code de sortie non nul si au moins un cas est en ÉCART franc.
    return 1 if n_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())
