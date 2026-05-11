# Media Hunt Session — 2026-05-11

## Goal
Break through paywalls/blocks to gather additional screenshots, reviews, and media for Motor City Online.

## Already Archived (Baseline)
- 14 article folders with text captures
- 51 images total (47 from Old-Games.RU, 2 Hidden Palace, 2 GameSpot thumbs)
- box-art.jpg

## Blocked Sources (Confirmed)

| Source | Block Type | Notes |
|--------|-----------|-------|
| Riot Pixels (72 screenshots) | Cloudflare 403 | Even r.jina.ai bypass fails |
| MobyGames | Cloudflare "Just a moment..." | r.jina.ai rate-limited (blocked until ~19:25 UTC) |
| Neoseeker | Cloudflare 403 | Direct fetch blocked |
| Giant Bomb | Cloudflare 403 | Direct fetch blocked |
| GameSpot (new screens 2006) | Cloudflare 403 | Direct fetch blocked |
| NFS Fandom Wiki | Cloudflare 403 | r.jina.ai works but no images extracted |
| Reddit r/MotorCityGameMemories | Cloudflare verification gate | Direct fetch blocked |
| Imgur gallery (NdyesiL) | Cloudflare | Title fetchable, images blocked |
| Newspapers.com (Tampa Tribune) | Paywall + JS | archive.org snapshot returns empty |

## Partial Successes / Leads

### Tampa Tribune Review
- **Score**: "D" (confirmed via Wikipedia citation)
- **Author**: Wes Phillips
- **Date**: Dec 7, 2001, p.127
- **Wikipedia citation**: https://www.newspapers.com/clip/98556533/the-tampa-tribune/
- **Archive.org snapshot**: https://web.archive.org/web/20220328101123/https://www.newspapers.com/clip/98556533/the-tampa-tribune/ (fetched but content empty — paywalled)
- **Status**: Text confirmed via Wikipedia but full article remains paywalled

### Computer Gaming World Issue 210
- **Date**: January 2002
- **MCO Preview**: Page 36
- **PDF URL**: https://www.cgwmuseum.org/galleries/issues/cgw_210.pdf (200+ MB, directly downloadable)
- **Archive.org OCR**: https://archive.org/stream/Computer_Gaming_World_Issue_210/Computer_Gaming_World_Issue_210_djvu.txt
- **Status**: PDF available but raw binary; OCR text confirms MCO on page 36 among previews (Aliens vs. Predator 2, MCO, No One Lives Forever 2, Dark Age of Camelot, Black & White: Creature Isle)
- **Action needed**: Download PDF, extract page 36 image/text

### PC Gamer Issue 87 (August 2001)
- **Archive.org OCR**: https://archive.org/stream/UneditedPCGamer_marktrade/PC_Gamer_087u_djvu.txt
- **Content**: E3 2001 wrap-up issue; MCO mentioned in coverage
- **Status**: OCR text fetched (20K chars) but MCO content not located in first portion; full text is massive
- **Action needed**: Search/grep OCR text for "Motor City" references

### Wikipedia Article (Confirmed Details)
- **Developer**: EA Seattle
- **Publisher**: EA Games
- **Platform**: Microsoft Windows
- **Release**: Oct 31, 2001
- **Shutdown**: Aug 29, 2003
- **Scores**: GameSpot 7.6/10, IGN 7.9/10, Tampa Tribune "D"
- **Box art URL**: https://upload.wikimedia.org/wikipedia/en/thumb/8/8b/Motor_City_Online_%28game_box_art%29.jpg/250px-Motor_City_Online_%28game_box_art%29.jpg
- **Full article via r.jina.ai**: successfully extracted text

### Reddit / Community Leads
- r/MotorCityGameMemories exists but blocked
- Imgur album "Anyone know the Motor City?" at https://imgur.com/gallery/NdyesiL (blocked)
- Facebook groups have MCO memories/photos (blocked)
- Steam Workshop has MCO car packs for other games (not original assets)

### Archive.org Game ISOs
- motor-city-online.-7z (644MB, archive.org)
- motor-city-online_202402 (357MB)
- motor-city-online-patch-2001-pc
- nfs-games-1996-2011-eu-region-versions/2001 - Motor City Online/

## Tools That Bypassed Blocks
- r.jina.ai/http://URL — works for some sites (Wikipedia, Fandom text) but not all
- archive.org OCR/djvu.txt — works for magazine scans
- archive.org direct PDF — works for cgwmuseum PDFs
- web_search with site:archive.org — limited success for specific game pages

## Tools That Failed
- Direct web_fetch to Cloudflare-protected sites
- r.jina.ai for Riot Pixels, MobyGames (rate limit)
- archive.org Wayback for Newspapers.com (paywall)

## Recommended Next Steps
1. Download CGW Issue 210 PDF, extract page 36 preview
2. Search PC Gamer 087 OCR text for all MCO references
3. Try different user-agent or textise dot iitty service for blocked sites
4. Check if Old-Games.RU has more albums/pages
5. Try Russian-language sources (AG.ru, StopGame, etc.)
6. Look for MCO community Discord/forum archives
