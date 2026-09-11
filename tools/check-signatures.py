#!/usr/bin/env python3
"""Verify signed commits/tags before publishing; reject backup refs."""

import argparse
import re
import subprocess
import sys


def git(*args):
    return subprocess.check_output(["git", *args], text=True).strip()


def verify(oid, kind, key):
    result = subprocess.run(
        ["git", "verify-" + kind, "--raw", oid], capture_output=True, text=True
    )
    fingerprints = re.findall(r"\[GNUPG:\] VALIDSIG ([0-9A-F]+)", result.stderr)
    if result.returncode or not any(value.endswith(key) for value in fingerprints):
        raise RuntimeError(f"{kind} {oid} lacks a valid signature from {key}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pre-push", action="store_true")
    parser.add_argument("revisions", nargs="*")
    args = parser.parse_args()
    if git("config", "--get", "gpg.format") != "openpgp":
        raise RuntimeError("OpenPGP signatures are required")
    key = git("config", "--get", "user.signingkey").replace(" ", "").rstrip("!").upper()
    if not re.fullmatch(r"[0-9A-F]{16,40}", key):
        raise RuntimeError(
            "Configure user.signingkey with the OpenPGP key ID/fingerprint"
        )
    commits = set()
    tags = set()
    if args.pre_push:
        for line in sys.stdin:
            local_ref, local_oid, remote_ref, remote_oid = line.split()
            if remote_ref.startswith("refs/backup/"):
                raise RuntimeError("Recovery refs must remain local")
            if set(local_oid) == {"0"}:
                continue
            if git("cat-file", "-t", local_oid) == "tag":
                tags.add(local_oid)
            elif local_ref.startswith("refs/tags/"):
                raise RuntimeError("Checkpoint tags must be signed annotated tags")
            tip = git("rev-parse", local_oid + "^{commit}")
            revision = [tip]
            known = subprocess.run(
                ["git", "cat-file", "-e", remote_oid], capture_output=True
            )
            if set(remote_oid) != {"0"} and known.returncode == 0:
                revision.append("^" + remote_oid + "^{commit}")
            else:
                revision += ["--not", "--remotes"]
            commits.update(git("rev-list", *revision).splitlines())
    else:
        for ref in args.revisions or ["HEAD"]:
            if ".." in ref:
                commits.update(git("rev-list", ref).splitlines())
            else:
                oid = git("rev-parse", ref)
                if git("cat-file", "-t", oid) == "tag":
                    tags.add(oid)
                commits.add(git("rev-parse", ref + "^{commit}"))
    for oid in sorted(commits):
        verify(oid, "commit", key)
    for oid in sorted(tags):
        verify(oid, "tag", key)
    print(f"PGP verified: {len(commits)} commits, {len(tags)} tags")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Signature check failed: {error}", file=sys.stderr)
        sys.exit(1)
