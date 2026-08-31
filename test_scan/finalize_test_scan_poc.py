from pathlib import Path

p = Path('src/image_scan_test.cpp')
text = p.read_text(encoding='utf-8')

if '#include <cstring>\n' not in text:
    text = text.replace('#include <cstdint>\n', '#include <cstdint>\n#include <cstring>\n', 1)
if '#include <iterator>\n' not in text:
    text = text.replace('#include <limits>\n', '#include <limits>\n#include <iterator>\n', 1)

old = '''std::wstring ReadText(HWND hwnd, int id) {
    const int n = GetWindowTextLengthW(GetDlgItem(hwnd, id));
    std::wstring text(static_cast<std::size_t>(std::max(0, n)), L'\\0');
    if (n > 0) GetDlgItemTextW(hwnd, id, text.data(), n + 1);
    return text;
}
'''
new = '''std::wstring ReadText(HWND hwnd, int id) {
    const int n = GetWindowTextLengthW(GetDlgItem(hwnd, id));
    if (n <= 0) return {};
    std::wstring text(static_cast<std::size_t>(n + 1), L'\\0');
    GetDlgItemTextW(hwnd, id, text.data(), n + 1);
    text.resize(static_cast<std::size_t>(n));
    return text;
}
'''
if old in text:
    text = text.replace(old, new, 1)
elif 'std::wstring text(static_cast<std::size_t>(n + 1)' not in text:
    raise SystemExit('ReadText hardening anchor missing')

p.write_text(text, encoding='utf-8')
print('TEST SCAN generated-source hardening PASS')
