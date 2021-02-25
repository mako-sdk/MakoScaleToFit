# MakoScaleToFit

Example code that demonstrates how to scale page content with the Mako SDK. Each PDF file in a specified folder is processed; each page in the document is scaled up or down to match the page size requested from the list.

```
Usage: <source folder> [<page size>]   Where: Page size chosen from the list below. 
                                              Default is US Letter (8.5in x 11in).

10X11                   10X14                   11X17                   12X11
15X11                   9X11                    A2                      A3
A3_EXTRA                A4                      A4SMALL                 A4_EXTRA
A4_PLUS                 A5                      A5_EXTRA                A6
A_PLUS                  B4                      B5                      B5_EXTRA
B6_JIS                  B_PLUS                  CSHEET                  DBL_JAPANESE_POSTCARD
DSHEET                  ENV_10                  ENV_11                  ENV_12
ENV_14                  ENV_9                   ENV_B4                  ENV_B5
ENV_B6                  ENV_C3                  ENV_C4                  ENV_C5
ENV_C6                  ENV_C65                 ENV_DL                  ENV_INVITE
ENV_ITALY               ENV_MONARCH             ENV_PERSONAL            ESHEET
EXECUTIVE               FANFOLD_LGL_GERMAN      FANFOLD_STD_GERMAN      FANFOLD_US
FOLIO                   ISO_B4                  JAPANESE_POSTCARD       JENV_CHOU3
JENV_CHOU4              JENV_KAKU2              JENV_KAKU3              JENV_YOU4
LEDGER                  LEGAL                   LEGAL_EXTRA             LETTER
LETTER_EXTRA            LETTER_PLUS             NOTE                    P16K
P32K                    P32KBIG                 PENV_1                  PENV_10
PENV_2                  PENV_3                  PENV_4                  PENV_5
PENV_6                  PENV_7                  PENV_8                  PENV_9
QUARTO                  STATEMENT               TABLOID                 TABLOID_EXTRA
```

## Description

The code demonstrates two techniques that are useful when manipulating pages and content, for example when imposing or repurposing content:

+ Adjusting page sizes
+ Scaling content

It does this on the pages in situ, rather than copying content to a new document, as seen in other examples.

### Adjusting page size

Page size can be set by setting the width and height of an `IDOMFixedPage`:

```C++
// Prepare to edit
IDOMFixedPagePtr content = page->edit();

// Set page dimensions to requested page size
content->setWidth(targetPageSize.width);
content->setHeight(targetPageSize.height);
```

This will adjust the PDF media size which will shrink the PDF dimension boxes (such as the crop box) if needed, but will not expand them automatically. But this is easy to do:

```C++
// Ensure cropbox (and other dimension boxes) match the new page size
FRect cropBox = FRect(0.0, 0.0, targetPageSize.width, targetPageSize.height);
content->setCropBox(cropBox);
content->setBleedBox(cropBox);
content->setTrimBox(cropBox);
content->setContentBox(cropBox);
```

### Scaling content

Scaling content is accomplished by moving content into a group with a scaling matrix. Note that `extractChild()` removes a node from the DOM tree for copying elsewhere, analogous to a Ctrl-X when you are text editing.

```C++
// Scale the page content by moving it into a group with a scaling matrix specified
IDOMGroupPtr scaleGroup = IDOMGroup::create(
    jawsMako, FMatrix(resizeScale, 0.0, 0.0, resizeScale, dx, dy));
IDOMNodePtr child = content->getFirstChild();
while (child)
{
    IDOMNodePtr next = child->getNextSibling();
    content->extractChild(child);
    scaleGroup->appendChild(child);
    child = next;
}

// Add the content to the page
content->appendChild(scaleGroup);
```

