"""Gera slides.pdf a partir de slides.html (Chromium via Playwright).

    pip install playwright && playwright install chromium
    python slides/gerar_pdf.py
"""
import pathlib
import sys

from playwright.sync_api import sync_playwright

PASTA = pathlib.Path(__file__).resolve().parent
chromium = sys.argv[1] if len(sys.argv) > 1 else None

with sync_playwright() as p:
    kwargs = {"executable_path": chromium, "args": ["--no-sandbox"]} if chromium else {}
    navegador = p.chromium.launch(**kwargs)
    pagina = navegador.new_page(viewport={"width": 1280, "height": 720})
    pagina.goto((PASTA / "slides.html").as_uri())
    pagina.pdf(path=str(PASTA / "slides.pdf"), width="1280px", height="720px",
               print_background=True, prefer_css_page_size=True)
    navegador.close()
print("slides.pdf gerado")
