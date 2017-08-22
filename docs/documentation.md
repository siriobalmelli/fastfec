---
title: Documentation
order: 1
---

# Documentation

Documentation is written in plain-text
	[Markdown](https://daringfireball.net/projects/markdown/) format.

[Jekyll](https://jekyllrb.com/) then transforms the Markdown
	source into a static web site.

Some of the documentation pages are run through
	[md2man](https://github.com/sunaku/md2man) by the build system,
	to generate man pages for install on target machines.

## Contributing

Your help in extending and correcting this documentation is quite welcome.

When writing or maintaing documentation, please adhere to the following:

-	All documentation files are in the [docs](docs/) dir, with an `.md` extension.
-	Links in markdown files are relative.
	The goal is that links work *both* when the markdown
		is being browsed directly on GitHub *and* in the generated Jekyll site.
-	Please put a `title:` frontmatter at the top of every file.
	If you're not familiar, *frontmatter* is a fancy way of referring to this:

```yaml
---
title: 'how to use Program X'
order: 1
---
```

-	The `order` directive above is used to set the ordering of your document
		in the left sidebar of the web page.
	A higher value is higher placement.

## Previewing Docs Locally

To build the documentation and preview it locally:

```bash
make
bundle exec jekyll serve
```
