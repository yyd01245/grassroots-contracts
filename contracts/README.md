# Grassroots

Grassroots is a crowdfunding development platform for EOSIO software. It allows account holders to vote native system tokens to fund projects, applications, campaigns, or even research and development.

Funds are sent to the contract and staked to the user's account. The user can then allocate an amount of tokens to projects as they see fit. A user can `reclaim` any unallocated system tokens out of the contract and back to their eosio.token account at any time.

In order to `unallocate` funds from a project, the user simply calls an action and specifies the proposal along with the amount to unallocate. Funds unallocated return to the contract wallet and can then either be reallocated to another project or reclaimed.

## Create an Account

Creating an account in Grassroots is as simple as calling the `grassroots::newaccount` action. This will create a zero-balance entry to store your future TLOS balance as well as dividends earned through use of the platform.

Additionally, users can create an account by simply making a regular `eosio.token::transfer` to the `grassrootsio` account with a memo of "newaccount". This will create a balance entry with RAM paid for by Grassroots, but will charge the creation of the account with a `0.1 TLOS` before placing the remainder of the transfer in the new account balance.

After creating a Grassroots account, all future `eosio.token::transfers` to `grassrootsio` will be caught by the contract and placed in the sender's Grassroots account.

## Creating a Project

Project creation in Grassroots is simple, just follow along this track:

### Project Setup

Creating a new project in Grassroots is easy and can be done by simply calling the `grassroots::newproject` action.

When setting string variables, use markdown format.

* `newproject`(name project_name, name category,   name creator, string title, string      description, string info_link, asset requested)

    `project_name` is the name of the new project. The project name must conform to the `eosio::name` encoding (a-z1-5).

    `category` is the category for the new project. Select the category from the available list.

    `creator` is the name of the creator, and must be the signer of this action.

    `title` is the title of the project.

    `description` is a brief description of the project.

    `info_link` is a link (ideally an ipfs link) for prosective contributors to learn more about the project.

    `requested` is the amount of `TLOS` requested to fund the project.

### Add Tiers

After creating the new project, the project creator can now add tiers to the project for contributors to purchase. Each tier can be seen as a purchase package when bundled with reward(s) outlined in the project description.

If a project is readied without any tiers, it cannot receive contributions and can only receive direct donations through the `grassroots::donate` action.

* `addtier`(name project_name, name creator, name tier_name, asset price, string description, uint16_t contributions)

    `project_name` is the name of the project to add the tier.

    `creator` is the name of the project creator. Only this account is authorized to add tiers.

    `tier_name` is the name of the new tier. It must be unique to other tiers in the project.

    `price` is the contribution price of the tier.

    `description` is a description of the tier and the reward(s) it offers.

    `contributions` is the total number of contributions accepted at this tier. Each contribution to this tier will decrement the remaining contributions.

### Make the Final Touches

If any edits need to be made they must be done before readying the project. Once a project is readied it can no longer be edited by the project creator until the contribution/donation period is over. To edit the project, simply call the `grassroots::editproject` action.

To leave a field unchanged, leave it blank. **In Development...**

* `editproject`(name project_name, name creator, string new_title, string new_desc, string new_link, asset new_requested)

    `project_name` is the name of the project to edit.

    `creator` is the name of the project creator. Only this account is authorized to edit the project.

    `new_title` is the new title.

    `new_desc` is the new description.

    `new_link` is the new info link.

    `new_requested` is the new requested amount for the project.

### Ready the Project

After project setup is complete, the last thing needed to do is to ready the project by calling the `grassroots::readyproject` action.

* `readyproject`(name project_name, name creator, uint8_t length_in_days)

    `project_name` is the name of the project to ready.

    `creator` is the name of the project creator. Only this account is authorized to ready the project.

    `length_in_days` is the length in days that the project contribution/donation period will be open, from the time the readyproject action is called.

### Wait for Contributions/Donations

After readying the project simply wait for contributions and donations to roll in. Make sure people know about your project!

### Close the Project

After the project's contribution/donation period is over, the project must be closed to determine whether the project was funded or not.

* `closeproject`

## Contributing to Projects

Below is a guide to interacting with the Grassroots platform and contributing to projects.

### Discover Projects

Browse a huge project catalogue with a wide variety of categories.

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

## Purchasing Tiers

To purchase a tier from a project simply call the `grassroots::contribute` action and supply the requisite information for your purchase.

* `contribute`(name project_name, name tier_name, name contributor, string memo)

    `project_name` is the name of the project to contribute to.

    `tier_name` is the name of the tier to purchase.

    `contirbutor` is the name of the account making the contribution.

    `memo` is a short message for the project creator.

Upon execution, the action will find the requested tier and bill the contirbutor's account for the tier price. This contribution will be returned to the contributor if the project fails to reach the requested funding level. **In Development**

## Make a Donation

To simply make a donation to a project without purchasing a tier, call the `grassroots::donate` action.

* `donate`(name project_name, name donor, asset amount, string memo)

    `project_name` is the name of the project to donate to.

    `donor` is the account name making the donation.

    `amount` is the amount in `TLOS` to donate.

    `memo` is a brief memo for the project creator.

### Refund a Contribution

* `refund`
