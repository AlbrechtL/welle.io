# Website

The welle.io website <https://www.welle.io> is built with
[MkDocs](https://www.mkdocs.org) and the
[Material for MkDocs](https://squidfunk.github.io/mkdocs-material/) theme from
the `docs/` folder of the repository and hosted with GitHub Pages. You are
welcome to improve it and send pull requests.

## Structure

| Path | Content |
|---|---|
| `mkdocs.yml` | Configuration: theme, colours, navigation |
| `docs/index.md` | Landing page (one page: features, screenshots, download, hardware, support) |
| `docs/overrides/home.html` | Hero at the top of the landing page |
| `docs/overrides/partials/source.html` | GitHub link in the header without loading stars and forks from `api.github.com` (not covered by the privacy policy) |
| `docs/stylesheets/extra.css` | Colours and styles of the landing page |
| `docs/user-guide/` | Installation, usage, welle-cli |
| `docs/devices/` | Hardware set-up |
| `docs/development/` | Building, contributing and technical documentation |
| `docs/faq.md`, `info.md`, `contact.md`, `impressum.md`, `datenschutzerklaerung.md` | Single pages |
| `docs/images/`, `docs/download/` | Screenshots and files to download |
| `docs/.well-known/`, `docs/CNAME` | Flathub verification of the app and the domain |

New pages have to be added to `nav` in `mkdocs.yml`. The URLs of the old
website (e.g. `/devices/rtl_sdr`, `/faq`, `/images/...`) are still used by
other sites and by the Flathub metainfo, so these files must not be renamed
or moved.

## Preview

```sh
python3 -m venv ~/.venvs/welle-docs
~/.venvs/welle-docs/bin/pip install -r docs/requirements.txt
~/.venvs/welle-docs/bin/mkdocs serve
```

Then open <http://127.0.0.1:8000/>; the page reloads when a file changes.
`mkdocs build --strict` builds the website into `site/` and fails on broken
links, like the deployment does.

## Deployment

The workflow `.github/workflows/pages.yml` builds the website and publishes
it with GitHub Pages when `docs/` or `mkdocs.yml` changes on `master`. It can
also be started by hand (*Actions* → *Website* → *Run workflow*), but only
publishes from `master`.

The `privacy` plugin downloads the fonts and other external assets at build
time, so visitors' browsers don't contact Google Fonts or other servers.
