# Grassroots Contract API

Grassroots is a crowdfunding development platform for EOSIO software. It allows account holders to vote native system tokens to fund projects, applications, campaigns, or even research and development.

Funds are sent to the contract and staked to the user's account. The user can then allocate an amount of tokens to projects as they see fit. A user can `withdraw` any unallocated system tokens out of the contract and back to their regular eosio.token account at any time.

In order to unallocate funds from a project, the user simply calls the `refund` action and specifies the proposal along with the tier level purchased. Refunds return to the contract wallet and can then either be contributed to another project or withdrawn.

## Create an Account

Creating an account in Grassroots is as simple as calling the `grassroots::newaccount` action. This will create a zero-balance entry to store your future TLOS balance as well as dividends earned through use of the platform.

* `newaccount(name new_account_name)`

    `new_account_name` is the name of the account to be created in Grassroots. Currently users must enter their own account names into this field.

Additionally, users can create an account by simply making a regular `eosio.token::transfer` to the `grassrootsio` account with a memo of "newaccount". This will create a balance entry with RAM paid for by Grassroots, but will charge an account creation fee of `0.1 TLOS` before placing the remainder of the transfer into the newly created account.

After creating a Grassroots account, all future `eosio.token::transfers` to `grassrootsio` will be caught by the contract and placed in the sender's Grassroots account.

A user's Grassroots account is their operating balance for all actions on the platform, meaning all contributions, donations, and fees are pulled from this account. If at any time a user experiences an `insufficient balance` error, they can simply transfer more `TLOS` to `grassrootsio` and debit their account.

## Creating a Project

Project creation in Grassroots is simple, just follow this track:

### Project Setup

Creating a new project in Grassroots is done by calling the `grassroots::newproject` action.

When setting string variables, please use markdown format. Grassroots React components are configured to parse markdown.

* ```newproject(name project_name, name category,   name creator, string title, string      description, string info_link, asset requested)```

    `project_name` is the name of the new project. The project name must conform to the `eosio::name` encoding (a-z1-5, max 12 characters).

    `category` is the category for the new project. Select the category from the available list below.

    `creator` is the name of the creator, and must be the signer of this action.

    `title` is the title of the project.

    `description` is a brief description of the project.

    `info_link` is a link (ideally an ipfs link) for prosective contributors to learn more about the project.

    `requested` is the amount of `TLOS` requested to fund the project.

Available categories: `games, apps, research, tools, environment, video, music, expansion, products, marketing`

### Add Tiers

After creating the new project, the project creator can now add tiers for contributors to purchase. Each tier can be seen as a package when bundled with reward(s) outlined in the project description.

Note that if a project is readied without any tiers, it cannot receive contributions and can only receive direct donations through the `grassroots::donate` action.

* `addtier`(name project_name, name creator, name tier_name, asset price, string description, uint16_t contributions)

    `project_name` is the name of the project to add the tier.

    `creator` is the name of the project creator. Only this account is authorized to add tiers.

    `tier_name` is the name of the new tier. It must be unique to other tiers in the project.

    `price` is the contribution price of the tier.

    `description` is a description of the tier and the reward(s) it offers.

    `contributions` is the total number of contributions accepted at this tier. Each contribution to this tier will decrement the remaining contributions.

* `removetier()`

    **In Development...**

### Make the Final Touches

If any edits need to be made they must be done before readying the project. Once a project is readied it can no longer be edited by the project creator until the contribution/donation period is over. To edit the project, simply call the `grassroots::editproject` action.

* `editproject`(name project_name, name creator, string new_title, string new_desc, string new_link, asset new_requested)

    `project_name` is the name of the project to edit.

    `creator` is the name of the project creator. Only this account is authorized to edit the project.

    `new_title` is the new title.

    `new_desc` is the new description.

    `new_link` is the new info link.

    `new_requested` is the new requested amount for the project.

To leave a field unchanged, type "none" in its field. **In Development...**

### Ready the Project

After project setup is complete, the final step is to ready the project by calling the `grassroots::readyproject` action.

Note that there is a non-refundable `25 TLOS` fee for readying a project.

* `readyproject(name project_name, name creator, uint8_t length_in_days)`

    `project_name` is the name of the project to be readied.

    `creator` is the name of the project creator. Only this account is authorized to ready the project.

    `length_in_days` is the number of days that the project contribution/donation period will be open, from the time the readyproject action is called.

### Wait for Contributions/Donations

After readying the project, simply wait for contributions and donations to roll in. Make sure people know about your project!

### Close the Project

After the project's contribution/donation period is over, the project must be closed to determine whether the project was funded or not.

This action can't be called until the contribution/donation period is over.

* `closeproject(name project_name, name creator)`

    `project_name` is the name of the project to close.

    `creator` is the name of the project creator. Only this account is authorized to close the project.

### Cancelling the Project

Projects can be cancelled at any time before being readied. The RAM spent by emplacing the project info is also returned to the project creator.

* `cancelproj(name project_name, name creator)`

    `project_name` is the name of the project being cancelled.

    `creator` is the name of the project creator. Only this account is authorized to cancel the project.

## Contributing to Projects

Below is a guide to interacting with the Grassroots platform and contributing to projects as a general user.

### Discover Projects

Browse a huge project catalogue with a wide variety of categories.

All: `cleos get table grassrootsio grassrootsio projects`

By Category: `cleos get table grassrootsio grassrootsio projects --lower category --key-type i64 --index 2`

* `games` : Create the next big blockchain-based game, or maybe just a game of checkers.

* `apps` : Fund development and deployment of all variety of apps.

* `research` : Fund a research project to benefit the community.

* `tools` : Create powerful tools for business and people.

* `environment` : Fund the next green initiative.

* `video` : Fund film and video projects.

* `music` : Fund music and audio projects.

* `expansion` : Fund expansion of the platform.

* `products` : Crowdfund a new product for the community or a business.

* `marketing` : Fund marketing campaigns that promote a business or community.

### Make a Contribution

To purchase a tier from a project simply call the `grassroots::contribute` action and supply the requisite information for your purchase.

* `contribute(name project_name, name tier_name, name contributor, string memo)`

    `project_name` is the name of the project receiving the contribution.

    `tier_name` is the name of the tier to purchase.

    `contirbutor` is the name of the account making the contribution.

    `memo` is a short message for the project creator.

Upon execution, the action will find the requested tier and bill the contributor's account for the tier price. This contribution can be refunded to the contributor as long as the project hasn't reached funding.

### Make a Donation

To simply donate to a project without making a purchase, call the `grassroots::donate` action.

* `donate(name project_name, name donor, asset amount, string memo)`

    `project_name` is the name of the project to donate to.

    `donor` is the account name making the donation.

    `amount` is the amount in `TLOS` to donate.

    `memo` is a brief memo for the project creator.

### Refund a Contribution

Contributions can be refunded back to the contributor at any time as long as the project's contribution period is still open and the project hasn't reached the requested funding level.

* `refund(name project_name, name contributor, name tier_name)`

    `project_name` is the name of the project to which the contribution was made.

    `contributor` is the name of the Grassroots account that made the contribution.

    `tier_name` is the name of the purchased tier to refund.

### Withdraw Funds

To withdraw funds from a Grassroots balance back to a regular `eosio.token` balance, simply call the `grassroots::withdraw` action. Users can withdraw an amount up to their Grassroots account balance.

* `withdraw(name account_name, asset amount)`

    `account_name` is the name of the Grassroots account to withdraw from. Only the owner of the account can withdraw from it.

    `amount` is the amount of `TLOS` to withdraw from the account.

Keep in mind that contributions and donations are pulled from your Grassroots balance, and users can always `refund` a contribution made to a project to recover their funds, provided the project hasn't reached funding.
