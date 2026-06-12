#include <mem/folio.h>
#include <mem/page.h>

bool page_is_folio_head(struct page *p) {
    return page_get_folio(p)->base_page == p;
}
