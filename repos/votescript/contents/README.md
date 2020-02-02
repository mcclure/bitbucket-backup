This is a set of PHP scripts that can be used to perform a ranked-ordering poll. During the poll, visitors to the vote page can rank up to 20 items of their choice; once the poll is done, it produces poll winners using multiple voting methods (first past the post, approval, instant runoff, and Borda count). It was created for a poll I ran personally with friends, so operating the poll is a little bit labor-intensive and only one poll can be run at a time.

**Table of Contents**

[TOC]

# Operation 

## Setup

The script requires a web server with PHP, and mysql. There are some setup steps.

1. Create a database on your MySQL server.
2. Connect to your new mysql database with the mysql command line tool. Execute all commands in the file "`setup.sql`".
3. Copy the file `config_sample.php` to `config.php`. Edit `config.php` and review all fields.

From here on out you will do the rest of the poll administration from a web browser.

## Starting the poll

The poll itself runs out of a script named `vote.php`. The rest of the scripts are "admin" scripts that will help you set up and monitor the poll.

The admin scripts have a concept of an **admin password**. This is a simple secret key which is stored in `config.php`. The admin scripts will refuse to load unless they find the admin password as a key in the GET string. In other words, if you wish to load results.php, and the password is "rutabaga", you should enter into your web browser:

`http://yourwebsite.com/vote/results.php?rutabaga=1`

This `?rutabaga=1` key should just be slapped onto the end of any admin URL you wish to load. (Note that your password should **not** be "rutabaga". See "On security" below.)

To start, you will want to load a script named `entry.php`. This will let you designate poll options. Open `entry.php` in your web browser with the password, enter your desired options in the box, and submit. `entry.php` will now show you the poll options currently in the database. If you realize you have made a mistake, you can re-run `entry.php` with the "clear first" box checked to erase all options.

Now that you have set up the options, you can begin the poll. Move the file `vote.php` to `index.php` on your server . The poll is now set up, and you can share the URL.

(I usually at this point, before I share the URL, do a couple test votes and then run `results.php` as described below, to make sure everything is working. If you do test vote, you can reset all votes afterward by running `delete from ftvote; delete from ftuser;` in mysql.)

## Collecting results

When the poll is done, move `index.php` back to `vote.php` and run `chmod -r vote.php` to prevent anyone voting after the deadline. This next step will take a bit, so I recommend creating a new `index.html` file with a note saying to check back in a few hours.

Now, you're not quite done yet. There is a problem. This script contains **no** protections against ballot stuffing. Nothing prevents someone from just voting over and over. Instead, the script records enough information to make it possible for you to detect simple forms of ballot stuffing after the fact. You are going to need to go through and do a manual check and prune for duplicate votes.

There is no script for this, so you're going to need to connect to your database with the mysql command line tool. Since you're going to be editing live database data, I suggest running `mysqldump` to make a backup of the database before you begin.

Here's the most basic ballot stuffing check you could run:

`select * from ftuser where hostname is not NULL order by hostname;`

This will show you all voters who completed a ballot, ordered by the IP address of the submitter. You want to scroll through this list and look for any consecutive entries with the same IP address. If this makes your eyes glaze over, there **is** one helper script named `check_ip.php` (requires the admin password) which will list all IPs that voted and how many times each one voted.

If you find any dupes, and in particular if you see more than two votes from a single IP, look closer. Multiple votes from an IP do not *necessarily* mean something is wrong! There might have been two voters living at the same address, for example. There is additional information you can look at to decide if there is a problem. For starters, the last field in this query, "initial", will show you what the HTTP referer used to originally access the page was. If this is the URL of your vote script, this means the user voted again immediately after voting the first time, which is very suspicious! Probably a malicious stuffer is more competent than that though, so you'll want to look closer. This will show you the ballots a particular IP submitted:

`select ftuser.id,ftvote.rate,ftgame.name from ftuser,ftvote,ftgame where ftvote.user=ftuser.id and ftvote.game=ftgame.id and ftuser.hostname="94.216.115.217" order by ftuser.id,ftvote.rate;`

Replace "`94.216.115.217`" with whatever the questionable IP was. If after reviewing you decide you need to delete a ballot, you can delete a single ballot's votes with:

`delete from ftvote where id=19634;`

Replace "`19634`" of course with the offending id.

As a note, you should keep in mind you are doing something obviously problematic here: You, the poll operator, are editing results. For this reason, you should be careful to only remove ballots if you're looking at a really unambiguous case of multiple voting, for example, two ballots with the same 20 selections in the same order. If you start having to make subtle rulings on edge cases, your personal biases will likely cause you to subconsciously favor ballots whose votes matched how you would have voted. If you do catch multiple voting, I suggest removing all votes from the offending IP *except* the most recent; some of these cases might not be an attempt to rig the poll, but rather an attempt by someone to earnestly revote because they made a mistake the first time.

## Publishing results

The results are reported through the scripts named beginning with "results". (**All results scripts require the admin password.**)

`results.php` is the interesting one; it provides a Borda count ranking of **all** entries which received votes, with a big, technicolor html grid showing a vote histogram. You probably want to edit the "`$mycolors`" array near the top of `results.php`; the first line contains special colors that the nine highest numbers in the results grid get labeled with, but fewer or more than nine such colors may be interesting depending on the number of options and the number of ballots you received.

`results_detail.php` shows the top 20 results using some alternate ranking methods. In addition to Borda are Approval, "First Past the Post" and Instant Runoff Vote rankings. (Wikipedia has explanations of all these terms.) Be careful when running this script, as it takes an **extremely** long time to complete. This is because the IRV implementation is not optimized and actually works by repeatedly creating and deleting MySQL tables. (You might need to fiddle with the `set_time_limit` call near the top of the script to make sure the script can finish.)

`results_specific.php` is an interesting variation of results.php: It filters and shows only votes whose HTTP referer matches a particular pattern. Note that the HTTP referer is not altogether trustworthy, it is easy to spoof and some personal privacy software strips it off. This script requires more manual intervention to run than the others. You will actually have to edit the script to specify the pattern. Look for the line starting with `$result =` near the top. I provided two possible examples.  

So, how to publish the results? Silly as this sounds, I just open the `results.php` in a web browser and select "Save As" to save an HTML file to my computer, then re-upload it as `index.html`. (I usually edit the HTML file first to remove the big block of Javascript at the top-- the javascript is unnecessary for the results pages, but it's currently part of the header). 

## Closing the poll

You're done! So that you can run another poll in future, I suggest backing up your results with `mysqldump` and then running these commands to reset the database:

`delete from ftvote; delete from ftgame; delete from ftuser;`

Or you could just re-run the `setup.sql` commands.

# Included files

Non-script files:

* `README.md`: You are reading this
* `setup.sql`: Sample SQL

Template files-- included by other files:

* `header.php`
* `footer.php`

Public scripts:

* `vote.php`

Admin scripts-- require admin password:

* `entry.php`
* `check_ip.php`

Results scripts-- require admin password:

* `results.php`, `results_detail.php`, `results_specific.php`, `results_raw.php`. See "Publishing results"

# On security

Please try to be aware of how insecure all this is. The administrator password, which means the potential to shut down the entire poll by deleting all the vote options, is transmitted in a URL. Unless you are using https, it will be transmitted across the internet in plaintext. Anyone with read access to files on your server will be able to open config.php and see both the admin password and your mysql password. Anyone with access to your httpd logs will be able to see the admin password. The admin password is unusually easy to dictionary-attack since you only have to request one URL over and over.

There are some steps you can do to mitigate this. 

1. If you are able, activate HTTPS.
2. Make absolutely certain the admin password is not a dictionary word or based on a dictionary word. If possible, it should be a long sequence of letters and numbers.
3. Deactivate any scripts you are not using, to reduce the attack surface. Run `chmod -r` on `entry.php` when you are done using it. `chmod -r` the results and check_ip scripts until you are ready to run them.

# Authorship / licensing

The files in this directory were written by Andi McClure <<andi.m.mcclure@gmail.com>>. I wrote all this around 2004 as a way of teaching myself PHP. In 2014 I cleaned it up just a *little* and wrote this README.

The contents of this directory are made available to you under the following license:

> Copyright (C) 2014 by Andi McClure
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

